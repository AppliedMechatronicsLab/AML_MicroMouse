#ifndef AML_LASERSENSOR_H
#define AML_LASERSENSOR_H

#include "stm32f4xx.h"
#include "vl53l0x_api.h"
#include "main.h"
#include "KalmanFilter.h"
#include "AML_Remote.h"



typedef enum
{
    FL,
    FF,
    FR,
    BR,
    BL
} LaserName;

void AML_LaserSensor_Setup();
void AML_LaserSensor_ReadAll();
int32_t AML_LaserSensor_ReadSingle(uint8_t name);

#endif