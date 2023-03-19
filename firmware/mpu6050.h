/*
 * MPU6050 driver for LWCLONEU2 
 * Copyright (C) 2023 Xavier Hosxe 
 *
 * LWCLONEU2  : Copyright (C) 2020 Andreas Dittrich <lwcloneu2@cithraidt.de>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DRIVER_MPU6050_H
#define DRIVER_MPU6050_H

#include "twi.h"

uint8_t mpu6050_init(void);
void mpu6050_ReadData(int *x, int *y);


void mpu6050_beginTransmission(uint8_t address);
uint8_t mpu6050_write(uint8_t data);
uint8_t mpu6050_endTransmission(uint8_t sendStop);
uint8_t mpu6050_endTransmissionStop(void);

#endif