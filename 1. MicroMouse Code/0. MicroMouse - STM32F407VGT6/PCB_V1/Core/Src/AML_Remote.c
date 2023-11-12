#include "AML_Remote.h"

extern UART_HandleTypeDef huart6;
extern int16_t debug[100];

uint8_t RemoteData[50];
uint8_t RemoteBuffer = 119;
uint8_t index = 0;
uint8_t ErrorString[] = "E";
uint8_t SuccessString[] = "S";

double ki, kp, kd;

void AML_Remote_Setup()
{
    HAL_UART_Receive_IT(&huart6, &RemoteBuffer, 1);
}

void AML_Remote_Handle()
{
    if (RemoteBuffer != 0)
    {
        RemoteData[index++] = RemoteBuffer;
    }
    else if (RemoteBuffer == 0)
    {
       
        sscanf((char *)RemoteData, "%lf %lf %lf\0", &ki, &kp, &kd);
        AML_MotorControl_PIDSetTunnings(kp, ki, kd);

        index = 0;
        memset(RemoteData, 0, sizeof(RemoteData));

        HAL_UART_Transmit(&huart6, SuccessString, 1, 100);
    }
    HAL_UART_Receive_IT(&huart6, &RemoteBuffer, 1);
}

void AML_Remote_SendData(uint8_t *data, uint8_t size)
{
    HAL_UART_Transmit(&huart6, data, size, 100);
}

