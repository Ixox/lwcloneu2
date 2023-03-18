#include "mpu6050.h"
#include "util/delay.h"
#include "comm.h"

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
  return mpuProblem;
}

uint8_t mpu6050_hasProblem() {
  return mpuProblem;
}

void mpu6050_ReadData(int16_t *x, int16_t *y, int16_t *z) {

  mpu6050_beginTransmission(MPU_ADDR);
  mpu6050_write(0x3B);
  mpu6050_endTransmission(0);
  mpu6050_beginTransmission(MPU_ADDR);

  uint8_t rxBuffer[6];
  uint8_t read = twi_readFrom(MPU_ADDR, rxBuffer, 6, true);
  // set rx buffer iterator vars
  
  if (mpuProblem != 0) {
    *x = 0;
    *y = 0;
    *z = 0;
  } else {

    *x = rxBuffer[0];
    *x = (*x << 8) + rxBuffer[1];
    *y = rxBuffer[2];
    *y = (*y << 8) + rxBuffer[3];
    *z = rxBuffer[4];
    *z = (*y << 8) + rxBuffer[5];
    value++;
    // if ((value % 100) == 0) {
    //    	DbgOut(DBGINFO, "x: %i", *x);
    // }
    // if ((value % 100) == 50) {
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


