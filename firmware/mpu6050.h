#ifndef DRIVER_MPU6050_H
#define DRIVER_MPU6050_H

#include "twi.h"

uint8_t mpu6050_init(void);
void mpu6050_ReadData(int16_t *x, int16_t *y, int16_t *z);


void mpu6050_beginTransmission(uint8_t address);
uint8_t mpu6050_write(uint8_t data);
uint8_t mpu6050_endTransmission(uint8_t sendStop);
uint8_t mpu6050_endTransmissionStop(void);

#endif