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

#include "mpu6050.h"
#include "util/delay.h"
#include "comm.h"
#include "millis_timer.h"

#define MPU_ADDR 0x68
#define BUFFER_LENGTH 32


uint8_t txAddress = 0;
uint8_t txBuffer[BUFFER_LENGTH];
uint8_t txBufferIndex = 0;
uint8_t txBufferLength = 0;

uint8_t mpuProblem = 0;


uint8_t mpu6050_init(void) {
 	DbgOut(DBGINFO, "mpu6050_init");
  txBufferIndex = 0;
  txBufferIndex = 0;
  txBufferLength = 0;
  twi_init();
  
  mpu6050_beginTransmission(MPU_ADDR);
  mpu6050_write(0x6B);
  mpu6050_write(0x00);
  mpuProblem =  mpu6050_endTransmissionStop();
  if (mpuProblem) {
 	  DbgOut(DBGINFO, "## mpu6050 NOT FOUND ##");
    millis_setLedTimer(200, 100);
  } else {
 	  DbgOut(DBGINFO, "mpu6050 detected ");
  }
  return mpuProblem;
}


void mpu6050_ReadData(int16_t *x, int16_t *y, int16_t *z) {
  // Moving average to smotth the result
  // + Auto calibration
  static int32_t mvAvrX = 0;
  static int32_t mvAvrY = 0;

  static cptDebug = 0;
  cptDebug++;
  // set rx buffer iterator vars
  
  if (mpuProblem != 0) {
    *x = 0;
    *y = 0;
    *z = 0;
  } else {

    mpu6050_beginTransmission(MPU_ADDR);
    mpu6050_write(0x3B);
    mpu6050_endTransmission(0);
    mpu6050_beginTransmission(MPU_ADDR);

    uint8_t rxBuffer[6];
    twi_readFrom(MPU_ADDR, rxBuffer, 6, true);

    int16_t accX = rxBuffer[0];
    accX = (accX << 8) + rxBuffer[1];
    int16_t accY = rxBuffer[2];
    accY = (accY << 8) + rxBuffer[3];
    int16_t accZ = rxBuffer[4];
    accZ = (accZ << 8) + rxBuffer[5];

    // mvAvr = (previous * 7 + new)  / 8
    mvAvrX = ((mvAvrX * 31) + accX ) >> 5;
    mvAvrY = ((mvAvrY * 31) + accY ) >> 5;

    *x = accX - mvAvrX;
    *y = accY - mvAvrY;
    *z = accZ;

    // if ((cptDebug % 200) == 0) {
    //    	DbgOut(DBGINFO, "x: %i", accX);
    //    	DbgOut(DBGINFO, "\t\t avr: %i", mvAvrX);
    // }
    // if ((cptDebug % 100) == 50) {
    //    	DbgOut(DBGINFO, "y: %i", *y);
    // }
  }
}


void mpu6050_beginTransmission(uint8_t address)
{
  txAddress = address;
  txBufferIndex = 0;
  txBufferLength = 0;
}

uint8_t mpu6050_write(uint8_t data)
{
  txBuffer[txBufferIndex++] = data;
  txBufferLength = txBufferIndex;
  return 1;
}



uint8_t mpu6050_endTransmission(uint8_t sendStop)
{
  uint8_t ret = twi_writeTo(txAddress, txBuffer, txBufferLength, 1, sendStop);
  txBufferIndex = 0;
  txBufferLength = 0;
  return ret;
}

uint8_t mpu6050_endTransmissionStop(void)
{
  return mpu6050_endTransmission(1);
}


