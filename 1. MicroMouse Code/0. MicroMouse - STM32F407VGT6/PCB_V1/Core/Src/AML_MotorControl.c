#include "AML_MotorControl.h"

#define Pi 3.14159265359  // Pi number
#define WheelDiameter 50  // mm
#define Ratio 90 / 14     // 90:14
#define PulsePerRound 190 // 190 pulse per round encoder
int32_t MouseSpeed = 20;  // % PWM

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim9;
extern int16_t debug[100];
extern uint8_t ReadButton;
extern VL53L0X_RangingMeasurementData_t SensorValue[7];
// int32_t LeftValue, RightValue;
// int32_t PreviousLeftEncoderValue = 0, PreviousRightEncoderValue = 0;

GPIO_PinState direction = GPIO_PIN_SET;

double Input_Left, Output_Left, Setpoint_Left;
double Input_Right, Output_Right, Setpoint_Right;

PID_TypeDef PID_SpeedLeft;
double Speed_Kp_Left = 1.0f;
double Speed_Ki_Left = 0.1f;
double Speed_Kd_Left = 0.1f;
double PreviouseLeftEncoderValue = 0;

PID_TypeDef PID_SpeedRight;
double Speed_Kp_Right = 1.0f;
double Speed_Ki_Right = 0.0f;
double Speed_Kd_Right = 0.f;
double PreviouseRightEncoderValue = 0;

PID_TypeDef PID_PositionLeft;
double Position_Kp = 0.005f;
double Position_Ki = 0.007f;
double Position_Kd = 0.0f;

PID_TypeDef PID_PositionRight;

double SetpointDistance;

///////////////////////////////////

PID_TypeDef PID_LeftWallFollow;
double LeftWallDistance;
double LeftWallFollow_Kp = 0.08f;
double LeftWallFollow_Ki = 0.0f;
double LeftWallFollow_Kd = 0.2f;

PID_TypeDef PID_RightWallFollow;
double RightWallDistance;
double RightWallFollow_Kp = 0.08f;
double RightWallFollow_Ki = 0.0f;
double RightWallFollow_Kd = 0.2f;

PID_TypeDef PID_MPUFollow;
double Input_MPUFollow, Output_MPUFollow, Setpoint_MPUFollow;
double MPUFollow_Kp = 0.1f;
double MPUFollow_Ki = 0.0f;
double MPUFollow_Kd = 0.2f;

//////////////////////////////////

double MinRotateSpeed = -25;
double MaxRotateSpeed = 25;

PID_TypeDef PID_TurnLeft;
double Input_TurnLeft, Output_TurnLeft, Setpoint_TurnLeft;
double TurnLeft_Kp = 0.35f;
double TurnLeft_Ki = 0.0f;
double TurnLeft_Kd = 0.015f;

PID_TypeDef PID_TurnRight;
double Input_TurnRight, Output_TurnRight, Setpoint_TurnRight;
double TurnRight_Kp = 0.35f;
double TurnRight_Ki = 0.0f;
double TurnRight_Kd = 0.015f;

uint16_t MinLeftWallDistance, MaxLeftWallDistance;
uint16_t MinRightWallDistance, MaxRightWallDistance;
uint16_t MinFrontWallDistance, MaxFrontWallDistance;

double OutputMin = 0;
double OutputMax = 70;

// int64_t PreviousTime = 0;

double ABS(double value)
{
    return value > 0 ? value : -value;
}

int32_t double_to_int32_t(double value)
{
    if (value < INT32_MIN)
    {
        return INT32_MIN;
    }
    else if (value > INT32_MAX)
    {
        return INT32_MAX;
    }
    else
    {
        return (int32_t)value;
    }
}

// void AML_MotorControl_PIDSetTunings(double Kp, double Ki, double Kd)
// {
//     PID_SpeedLeft.Kp = Kp;
//     PID_SpeedLeft.Ki = Ki;
//     PID_SpeedLeft.Kd = Kd;

//     PID_SpeedRight.Kp = Kp;
//     PID_SpeedRight.Ki = Ki;
//     PID_SpeedRight.Kd = Kd;
// }

void AML_MotorControl_PIDSetTunnings(double Kp, double Ki, double Kd)
{
    PID_TypeDef *pid = &PID_TurnLeft;
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
}

void AML_MotorControl_PIDSetSampleTime(uint32_t NewSampleTime)
{
    PID_SpeedLeft.SampleTime = NewSampleTime;
    PID_SpeedRight.SampleTime = NewSampleTime;

    PID_PositionLeft.SampleTime = NewSampleTime;
    PID_PositionRight.SampleTime = NewSampleTime;

    PID_LeftWallFollow.SampleTime = NewSampleTime;
    PID_RightWallFollow.SampleTime = NewSampleTime;

    PID_TurnLeft.SampleTime = NewSampleTime;
    PID_TurnRight.SampleTime = NewSampleTime;

    PID_MPUFollow.SampleTime = NewSampleTime;
}

void AML_MotorControl_PIDSetOutputLimits(double Min, double Max)
{
    PID_SpeedLeft.OutMin = Min;
    PID_SpeedLeft.OutMax = Max;

    PID_SpeedRight.OutMin = Min;
    PID_SpeedRight.OutMax = Max;

    PID_PositionLeft.OutMin = Min;
    PID_PositionLeft.OutMax = Max;

    PID_PositionRight.OutMin = Min;
    PID_PositionRight.OutMax = Max;

    PID_LeftWallFollow.OutMin = -15;
    PID_LeftWallFollow.OutMax = 15;

    PID_RightWallFollow.OutMin = -15;
    PID_RightWallFollow.OutMax = 15;

    PID_TurnLeft.OutMin = MinRotateSpeed;
    PID_TurnLeft.OutMax = MaxRotateSpeed;

    PID_TurnRight.OutMin = MinRotateSpeed;
    PID_TurnRight.OutMax = MaxRotateSpeed;

    PID_MPUFollow.OutMin = -15;
    PID_MPUFollow.OutMax = 15;
}

void AML_MotorControl_PIDSetMode(PIDMode_TypeDef Mode)
{
    PID_SetMode(&PID_SpeedLeft, Mode);
    PID_SetMode(&PID_SpeedRight, Mode);

    PID_SetMode(&PID_PositionLeft, Mode);
    PID_SetMode(&PID_PositionRight, Mode);

    PID_SetMode(&PID_LeftWallFollow, Mode);
    PID_SetMode(&PID_RightWallFollow, Mode);

    PID_SetMode(&PID_TurnLeft, Mode);
    PID_SetMode(&PID_TurnRight, Mode);

    PID_SetMode(&PID_MPUFollow, Mode);
}

void AML_MotorControl_PIDSetup()
{
    PID_Init(&PID_SpeedLeft);
    PID_Init(&PID_SpeedRight);

    PID_Init(&PID_PositionLeft);
    PID_Init(&PID_PositionRight);

    PID_Init(&PID_LeftWallFollow);
    PID_Init(&PID_RightWallFollow);

    PID_Init(&PID_TurnLeft);
    PID_Init(&PID_TurnRight);

    PID_Init(&PID_MPUFollow);

    PID(&PID_SpeedLeft, &Input_Left, &Output_Left, &Setpoint_Left, Speed_Kp_Left, Speed_Ki_Left, Speed_Kd_Left, _PID_P_ON_E, PID_SpeedLeft.ControllerDirection);
    PID(&PID_SpeedRight, &Input_Right, &Output_Right, &Setpoint_Right, Speed_Kp_Right, Speed_Ki_Right, Speed_Kd_Right, _PID_P_ON_E, PID_SpeedRight.ControllerDirection);

    PID(&PID_PositionLeft, &Input_Left, &Output_Left, &Setpoint_Left, Position_Kp, Position_Ki, Position_Kd, _PID_P_ON_E, PID_PositionLeft.ControllerDirection);
    PID(&PID_PositionRight, &Input_Right, &Output_Right, &Setpoint_Right, Position_Kp, Position_Ki, Position_Kd, _PID_P_ON_E, PID_PositionRight.ControllerDirection);

    PID(&PID_LeftWallFollow, &LeftWallDistance, &Output_Left, &SetpointDistance, LeftWallFollow_Kp, LeftWallFollow_Ki, LeftWallFollow_Kd, _PID_P_ON_E, PID_LeftWallFollow.ControllerDirection);
    PID(&PID_RightWallFollow, &RightWallDistance, &Output_Right, &SetpointDistance, RightWallFollow_Kp, RightWallFollow_Ki, RightWallFollow_Kd, _PID_P_ON_E, PID_RightWallFollow.ControllerDirection);

    PID(&PID_TurnLeft, &Input_TurnLeft, &Output_TurnLeft, &Setpoint_TurnLeft, TurnLeft_Kp, TurnLeft_Ki, TurnLeft_Kd, _PID_P_ON_E, PID_TurnLeft.ControllerDirection);
    PID(&PID_TurnRight, &Input_TurnRight, &Output_TurnRight, &Setpoint_TurnRight, TurnRight_Kp, TurnRight_Ki, TurnRight_Kd, _PID_P_ON_E, PID_TurnRight.ControllerDirection);

    PID(&PID_MPUFollow, &Input_MPUFollow, &Output_MPUFollow, &Setpoint_MPUFollow, MPUFollow_Kp, MPUFollow_Ki, MPUFollow_Kd, _PID_P_ON_E, PID_MPUFollow.ControllerDirection);

    // AML_MotorControl_PIDSetTunings();

    AML_MotorControl_PIDSetSampleTime(20);
    AML_MotorControl_PIDSetOutputLimits(OutputMin, OutputMax);
    AML_MotorControl_PIDSetMode(_PID_MODE_AUTOMATIC);
}

void AML_MotorControl_PIDReset(PID_TypeDef *uPID)
{
    uPID->OutputSum = 0;
    uPID->LastInput = 0;
    uPID->LastTime = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// init motor control
void AML_MotorControl_Setup(void)
{
    HAL_TIM_Base_Start(&htim2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, GPIO_PIN_SET);

    AML_MotorControl_PIDSetup();
}

void AML_MotorControl_LeftPWM(int32_t PWMValue)
{
    if (PWMValue > 100)
    {
        PWMValue = 100;
    }
    else if (PWMValue < -100)
    {
        PWMValue = -100;
    }

    if (PWMValue > 0)
    {
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, direction);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, !direction);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, PWMValue);
    }
    else if (PWMValue < 0)
    {
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, !direction);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, direction);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, -PWMValue);
    }
    else if (PWMValue == 0)
    {
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
    }
}

void AML_MotorControl_RightPWM(int32_t PWMValue)
{
    PWMValue = (int32_t)(PWMValue * 1);

    if (PWMValue > 100)
    {
        PWMValue = 100;
    }
    else if (PWMValue < -100)
    {
        PWMValue = -100;
    }

    if (PWMValue > 0)
    {
        HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, !direction);
        HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, direction);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, PWMValue);
    }
    else if (PWMValue < 0)
    {
        HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, direction);
        HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, !direction);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, -PWMValue);
    }
    else if (PWMValue == 0)
    {
        HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
    }
}

void AML_MotorControl_SetDirection(GPIO_PinState dir)
{
    direction = dir;
}

void AML_MotorControl_ToggleDirection(void)
{
    direction = ~direction;
}

void AML_MotorControl_SetMouseSpeed(int32_t speed)
{
    MouseSpeed = speed;
}

void AML_MotorControl_SetLeftSpeed(double speed, GPIO_PinState direction)
{
    Setpoint_Left = speed * Ratio;

    double CurrentValue = (double)AML_Encoder_GetLeftValue();
    Input_Left = ABS((double)(CurrentValue - PreviouseLeftEncoderValue) / PulsePerRound);

    PreviouseLeftEncoderValue = CurrentValue;

    PID_Compute(&PID_SpeedLeft);

    // AML_MotorControl_LeftPWM(Output_Left * direction);
}

void AML_MotorControl_SetRightSpeed(double speed, GPIO_PinState direction)
{
    Setpoint_Right = speed * Ratio;
    Input_Right = ABS((double)AML_Encoder_GetRightValue() / PulsePerRound);
    // AML_Encoder_ResetRightValue();

    PID_Compute(&PID_SpeedRight);

    AML_MotorControl_RightPWM(Output_Right * direction);
}

void AML_MotorControl_Stop(void)
{
    AML_MotorControl_LeftPWM(0);
    AML_MotorControl_RightPWM(0);
}

void AML_MotorControl_ShortBreak(char c)
{
    uint8_t TurnSpeed = 30;
    uint8_t GoStraightSpeed = 40;

    if (c == 'L')
    {

        AML_MotorControl_LeftPWM(TurnSpeed);
        AML_MotorControl_RightPWM(-TurnSpeed);
    }
    else if (c == 'R')
    {
        AML_MotorControl_LeftPWM(-TurnSpeed);
        AML_MotorControl_RightPWM(TurnSpeed);
    }
    else if (c == 'F')
    {
        AML_MotorControl_LeftPWM(-GoStraightSpeed);
        AML_MotorControl_RightPWM(-GoStraightSpeed);
    }
    else if (c == 'B')
    {
        AML_MotorControl_LeftPWM(GoStraightSpeed);
        AML_MotorControl_RightPWM(GoStraightSpeed);
    }

    HAL_Delay(20);
    AML_MotorControl_Stop();
}

void AML_MotorControl_SetCenterPosition(void)
{
    MinLeftWallDistance = AML_LaserSensor_ReadSingleWithFillter(BL);
    MinRightWallDistance = AML_LaserSensor_ReadSingleWithFillter(BR);

    HAL_Delay(22);

    MinLeftWallDistance = AML_LaserSensor_ReadSingleWithFillter(BL);
    MinRightWallDistance = AML_LaserSensor_ReadSingleWithFillter(BR);

    HAL_Delay(22);

    MinLeftWallDistance = AML_LaserSensor_ReadSingleWithFillter(BL);
    MinRightWallDistance = AML_LaserSensor_ReadSingleWithFillter(BR);
}

void AML_MotorControl_SetLeftWallValue(void)
{
    MinLeftWallDistance = AML_LaserSensor_ReadSingleWithFillter(BL);
    MaxRightWallDistance = AML_LaserSensor_ReadSingleWithFillter(BR);

    MinFrontWallDistance = AML_LaserSensor_ReadSingleWithFillter(FF);

    HAL_Delay(22);

    MinLeftWallDistance = AML_LaserSensor_ReadSingleWithFillter(BL);
    MaxRightWallDistance = AML_LaserSensor_ReadSingleWithFillter(BR);

    MinFrontWallDistance = AML_LaserSensor_ReadSingleWithFillter(FF);

    HAL_Delay(22);

    MinLeftWallDistance = AML_LaserSensor_ReadSingleWithFillter(BL);
    MaxRightWallDistance = AML_LaserSensor_ReadSingleWithFillter(BR);

    MinFrontWallDistance = AML_LaserSensor_ReadSingleWithFillter(FF);
}

void AML_MotorControl_SetRightWallValue(void)
{
    MaxLeftWallDistance = AML_LaserSensor_ReadSingleWithFillter(BL);
    MinRightWallDistance = AML_LaserSensor_ReadSingleWithFillter(BR);

    MinFrontWallDistance = AML_LaserSensor_ReadSingleWithFillter(FF);

    HAL_Delay(22);

    MaxLeftWallDistance = AML_LaserSensor_ReadSingleWithFillter(BL);
    MinRightWallDistance = AML_LaserSensor_ReadSingleWithFillter(BR);

    MinFrontWallDistance = AML_LaserSensor_ReadSingleWithFillter(FF);

    HAL_Delay(22);

    MaxLeftWallDistance = AML_LaserSensor_ReadSingleWithFillter(BL);
    MinRightWallDistance = AML_LaserSensor_ReadSingleWithFillter(BR);

    MinFrontWallDistance = AML_LaserSensor_ReadSingleWithFillter(FF);
}

void AML_MotorControl_LeftWallFollow(void)
{
    AML_DebugDevice_TurnOnLED(2);

    AML_DebugDevice_TurnOffLED(3);
    AML_DebugDevice_TurnOffLED(4);
    AML_DebugDevice_TurnOffLED(5);
    AML_DebugDevice_TurnOffLED(6);
    AML_DebugDevice_TurnOffLED(7);

    LeftWallDistance = (double)AML_LaserSensor_ReadSingleWithFillter(BL);

    // SetpointDistance = (double)(MinLeftWallDistance) + 35;
    SetpointDistance = (double)(MinLeftWallDistance);

    PID_Compute(&PID_LeftWallFollow);

    AML_MotorControl_LeftPWM((MouseSpeed + (int32_t)Output_Left));
    AML_MotorControl_RightPWM((MouseSpeed - (int32_t)Output_Left));
}

void AML_MotorControl_RightWallFollow(void)
{
    AML_DebugDevice_TurnOnLED(3);

    AML_DebugDevice_TurnOffLED(2);
    AML_DebugDevice_TurnOffLED(4);
    AML_DebugDevice_TurnOffLED(5);
    AML_DebugDevice_TurnOffLED(6);
    AML_DebugDevice_TurnOffLED(7);

    RightWallDistance = (double)AML_LaserSensor_ReadSingleWithFillter(BR);

    // SetpointDistance = (double)(MinRightWallDistance) + 35;
    SetpointDistance = (double)(MinRightWallDistance);

    PID_Compute(&PID_RightWallFollow);

    AML_MotorControl_LeftPWM((MouseSpeed - (int32_t)Output_Right));
    AML_MotorControl_RightPWM((MouseSpeed + (int32_t)Output_Right));
}

void AML_MotorControl_MPUFollow(void)
{
    AML_DebugDevice_TurnOnLED(4);

    AML_DebugDevice_TurnOffLED(2);
    AML_DebugDevice_TurnOffLED(3);
    AML_DebugDevice_TurnOffLED(5);
    AML_DebugDevice_TurnOffLED(6);
    AML_DebugDevice_TurnOffLED(7);

    Setpoint_MPUFollow = 0.0f;

    Input_MPUFollow = (double)AML_MPUSensor_GetAngle();

    PID_Compute(&PID_MPUFollow);

    AML_MotorControl_LeftPWM((MouseSpeed - (int32_t)Output_MPUFollow));
    AML_MotorControl_RightPWM((MouseSpeed + (int32_t)Output_MPUFollow));
}

void AML_MotorControl_GoStraight(void)
{
    if (AML_LaserSensor_ReadSingleWithFillter(BL) < LEFT_WALL)
    {
        AML_MotorControl_LeftWallFollow();
    }
    else if (AML_LaserSensor_ReadSingleWithFillter(BR) < RIGHT_WALL)
    {
        AML_MotorControl_RightWallFollow();
    }
    else if (AML_LaserSensor_ReadSingleWithFillter(BL) > NO_LEFT_WALL && AML_LaserSensor_ReadSingleWithFillter(BR) > NO_RIGHT_WALL)
    {
        AML_MotorControl_MPUFollow();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // UNUSED(htim);
    if (htim->Instance == htim9.Instance)
    {
        AML_MotorControl_GoStraight();
        // Move();
        // debug[10]++;
    }
    // debug[11]++;
}

void AML_MotorControl_TurnOnWallFollow(void)
{
    HAL_TIM_Base_Start_IT(&htim9);
}

void AML_MotorControl_TurnOffWallFollow(void)
{
    HAL_TIM_Base_Stop_IT(&htim9);
}

void AML_MotorControl_MoveForward_mm(uint16_t distance)
{
    AML_Encoder_ResetLeftValue();
    AML_Encoder_ResetRightValue();
    // AML_MotorControl_TurnOnWallFollow();

    uint16_t ticks = (uint16_t)((double)distance / (Pi * WheelDiameter) * PulsePerRound * Ratio);

    while (ReadButton != 2 && AML_Encoder_GetLeftValue() < ticks)
    {
        //  AML_MotorControl_GoStraight();
        AML_MotorControl_LeftPWM(MouseSpeed);
        AML_MotorControl_RightPWM(MouseSpeed);
    }

    // AML_MotorControl_TurnOffWallFollow();

    // AML_MotorControl_ShortBreak('F');

    AML_Encoder_ResetLeftValue();
}

void AML_MotorControl_AdvanceTicks(int16_t ticks)
{
    AML_Encoder_ResetLeftValue();
    AML_MPUSensor_ResetAngle();

    AML_MotorControl_TurnOnWallFollow();

    while (ReadButton != 2 && AML_Encoder_GetLeftValue() < ticks)
    {
    }

    AML_MotorControl_TurnOffWallFollow();

    AML_Encoder_ResetLeftValue();
}

void AML_MotorControl_TurnLeft90(void)
{
    AML_MPUSensor_ResetAngle();

    Input_TurnLeft = (double)AML_MPUSensor_GetAngle();
    Setpoint_TurnLeft = Input_Left + 90.0f;

    HAL_Delay(200);

    uint32_t InitTime = HAL_GetTick();

    while ((ReadButton != 2) && (ABS(Input_TurnLeft - Setpoint_TurnLeft) > 10.0f) && (HAL_GetTick() - InitTime < 1500))
    {
        Input_TurnLeft = AML_MPUSensor_GetAngle();
        PID_Compute(&PID_TurnLeft);

        AML_MotorControl_LeftPWM(-(int32_t)Output_TurnLeft);
        AML_MotorControl_RightPWM((int32_t)Output_TurnLeft);
    }

    AML_MotorControl_ShortBreak('L');

    AML_MPUSensor_ResetAngle();

    HAL_Delay(200);
}

void AML_MotorControl_TurnRight90(void)
{
    AML_MPUSensor_ResetAngle();

    Input_TurnRight = (double)AML_MPUSensor_GetAngle();
    Setpoint_TurnRight = Input_Right - 90.0f;

    HAL_Delay(200);
    uint32_t InitTime = HAL_GetTick();

    while ((ReadButton != 2) && (ABS(Input_TurnRight - Setpoint_TurnRight) > 10.0f) && (HAL_GetTick() - InitTime < 1500))
    {
        Input_TurnRight = AML_MPUSensor_GetAngle();
        PID_Compute(&PID_TurnRight);

        AML_MotorControl_LeftPWM(-(int32_t)Output_TurnRight);
        AML_MotorControl_RightPWM((int32_t)Output_TurnRight);
    }

    AML_MotorControl_ShortBreak('R');

    AML_MPUSensor_ResetAngle();

    HAL_Delay(200);
}

void AML_MotorControl_TurnLeft180(void)
{
    AML_MPUSensor_ResetAngle();

    Input_TurnLeft = (double)AML_MPUSensor_GetAngle();
    Setpoint_TurnLeft = Input_Left + 180.0f;

    AML_MotorControl_TurnOffWallFollow();

    HAL_Delay(200);
    uint32_t InitTime = HAL_GetTick();

    while ((ReadButton != 2) && (ABS(Input_TurnLeft - Setpoint_TurnLeft) > 10.0f) && (HAL_GetTick() - InitTime < 1500))
    {
        Input_TurnLeft = AML_MPUSensor_GetAngle();
        PID_Compute(&PID_TurnLeft);

        AML_MotorControl_LeftPWM(-(int32_t)Output_TurnLeft);
        AML_MotorControl_RightPWM((int32_t)Output_TurnLeft);
    }

    AML_MotorControl_ShortBreak('L');

    AML_MPUSensor_ResetAngle();

    HAL_Delay(200);

}

void AML_MotorControl_TurnRight180(void)
{
    AML_MPUSensor_ResetAngle();

    Input_TurnRight = (double)AML_MPUSensor_GetAngle();
    Setpoint_TurnRight = Input_Right - 180.0f;
    // HAL_Delay(1000);

    AML_MotorControl_TurnOffWallFollow();
    AML_MotorControl_ShortBreak('F');

    HAL_Delay(200);
    uint32_t InitTime = HAL_GetTick();

    while ((ReadButton != 2) && (ABS(Input_TurnRight - Setpoint_TurnRight) > 10.0f) && (HAL_GetTick() - InitTime < 1500))
    {
        Input_TurnRight = AML_MPUSensor_GetAngle();
        PID_Compute(&PID_TurnRight);

        AML_MotorControl_LeftPWM(-(int32_t)Output_TurnRight);
        AML_MotorControl_RightPWM((int32_t)Output_TurnRight);
    }

    AML_MotorControl_ShortBreak('R');

    AML_MPUSensor_ResetAngle();

    HAL_Delay(200);

    // AML_MotorControl_MoveForward_mm(60);
    // AML_MotorControl_Stop();
    // AML_MotorControl_PIDReset(&PID_TurnRight);
}

void AML_MotorControl_LeftStillTurn(void)
{
    AML_DebugDevice_TurnOnLED(5);

    AML_DebugDevice_TurnOffLED(2);
    AML_DebugDevice_TurnOffLED(3);
    AML_DebugDevice_TurnOffLED(4);

    AML_DebugDevice_BuzzerBeep(20);

    AML_MotorControl_AdvanceTicks((int16_t)(FLOOD_ENCODER_TICKS_ONE_CELL / 2));
    AML_MotorControl_ShortBreak('F');

    AML_MotorControl_TurnLeft90();

    AML_MotorControl_AdvanceTicks((int16_t)(FLOOD_ENCODER_TICKS_ONE_CELL / 2));

    AML_MotorControl_TurnOnWallFollow();
    AML_DebugDevice_TurnOffLED(5);
}

void AML_MotorControl_RightStillTurn(void)
{
    AML_DebugDevice_TurnOnLED(6);

    AML_DebugDevice_TurnOffLED(2);
    AML_DebugDevice_TurnOffLED(3);
    AML_DebugDevice_TurnOffLED(4);
    AML_DebugDevice_TurnOffLED(5);

    AML_DebugDevice_BuzzerBeep(20);

    AML_MotorControl_AdvanceTicks((int16_t)(FLOOD_ENCODER_TICKS_ONE_CELL / 2));
    AML_MotorControl_ShortBreak('F');
    
    AML_MotorControl_TurnRight90();

    AML_MotorControl_AdvanceTicks((int16_t)(FLOOD_ENCODER_TICKS_ONE_CELL / 2));

    AML_MotorControl_TurnOnWallFollow();
    AML_DebugDevice_TurnOffLED(6);
}

void AML_MotorControl_BackStillTurn(void)
{
    AML_DebugDevice_TurnOnLED(7);

    AML_DebugDevice_TurnOffLED(2);
    AML_DebugDevice_TurnOffLED(3);
    AML_DebugDevice_TurnOffLED(4);
    AML_DebugDevice_TurnOffLED(5);
    AML_DebugDevice_TurnOffLED(6);

    AML_DebugDevice_BuzzerBeep(20);

    AML_MotorControl_TurnOffWallFollow();
    AML_MotorControl_ShortBreak('F');

    AML_MotorControl_TurnLeft180();

    AML_MotorControl_TurnOnWallFollow();

    AML_DebugDevice_TurnOffLED(7);
}

/////////////////////////////////////////////////////////////

void AML_MotroControl_FloodLeftStillTurn(void)
{
    AML_DebugDevice_TurnOnLED(5);

    AML_DebugDevice_TurnOffLED(2);
    AML_DebugDevice_TurnOffLED(3);
    AML_DebugDevice_TurnOffLED(4);

    AML_DebugDevice_BuzzerBeep(20);

    AML_MotorControl_AdvanceTicks((int16_t)(FLOOD_ENCODER_TICKS_ONE_CELL / 2));
    AML_MotorControl_ShortBreak('F');

    AML_MotorControl_TurnLeft90();

    AML_MotorControl_AdvanceTicks((int16_t)(FLOOD_ENCODER_TICKS_ONE_CELL / 2));

    AML_MotorControl_TurnOnWallFollow();
    AML_DebugDevice_TurnOffLED(5);
}
