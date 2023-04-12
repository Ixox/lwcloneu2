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

// Lots of code copied from https://os.mbed.com/users/mjr/code/Pinscape_Controller_V2/
// For compatibility


#include "util/delay.h"
#include <avr/interrupt.h>

#include "comm.h"
#include "led_toys_pwm.h"
#include "mpu6050.h"

#define MPU_ADDR 0x68
#define BUFFER_LENGTH 32

#define MPU_PWR_MGMT_1 0x6B
#define MPU_SAMPLE_RATE_DIVIDER 0x19
#define MPU_FIFO 0x23
#define MPU_USER_CTRL 0x6A
#define MPU_FIFO 0x23
#define MPU_FIFO_COUNT_H 0x72
#define MPU_FIFO_COUNT_L 0x73
#define MPU_FIFO_R_W 0x74
#define MPU_ACCEL_XOUT_H 0x3B

uint8_t txAddress = 0;
uint8_t txBuffer[BUFFER_LENGTH];
uint8_t txBufferIndex = 0;
uint8_t txBufferLength = 0;

uint8_t mpuProblem = 0;

// last raw acceleration readings
int ax_, ay_, az_;

// integrated velocity reading since last get()
int vx_, vy_;

// Calibration reference point for accelerometer.  This is the
// average reading on the accelerometer when in the neutral position
// at rest.
int cx_, cy_;

// running sum of readings since last get()
int32_t xSum_, ySum_;

// number of readings since last get()
int nSum_;

// Auto-centering history.  This is a separate history list that
// records results spaced out sparesely over time, so that we can
// watch for long-lasting periods of rest.  When we observe nearly
// no motion for an extended period (on the order of 5 seconds), we
// take this to mean that the cabinet is at rest in its neutral
// position, so we take this as the calibration zero point for the
// accelerometer.  We update this history continuously, which allows
// us to continuously re-calibrate the accelerometer.  This ensures
// that we'll automatically adjust to any actual changes in the
// cabinet's orientation (e.g., if it gets moved slightly by an
// especially strong nudge) as well as any systematic drift in the
// accelerometer measurement bias (e.g., from temperature changes).
uint8_t iAccPrv_, nAccPrv_;


// time in us between auto-centering incremental checks
uint32_t autoCenterCheckTime_;

#define JOYMAX 4096

void get(int *x, int *y);
int rawToReport(int v);
bool flushFifo();

uint8_t mpu6050_init(void) {
 	DbgOut(DBGINFO, "mpu6050_init");
  txBufferIndex = 0;
  txBufferIndex = 0;
  txBufferLength = 0;

  twi_init();
  sei();

  iAccPrv_ = nAccPrv_ = 0;

  // WHO AM I 
  mpu6050_beginTransmission(MPU_ADDR);
  mpu6050_write(0x75);
  mpu6050_endTransmission(0);
  mpu6050_beginTransmission(MPU_ADDR);

  uint8_t whoAmI = 0;
  twi_readFrom(MPU_ADDR, &whoAmI, 1, true);
  if (whoAmI != 0x68) {
 	  DbgOut(DBGINFO, "## It' not a MPU6050##");
    led_setBlinkTimer(200, 100);
    return 1;  
  } else {
 	  DbgOut(DBGINFO, "MPU6050 detected ");    
  }

  // RESET
  mpu6050_beginTransmission(MPU_ADDR);
  mpu6050_write(MPU_PWR_MGMT_1);
  mpu6050_write(0x00);
  mpuProblem =  mpu6050_endTransmissionStop();
  if (mpuProblem) {
 	  DbgOut(DBGINFO, "## Could not reset MPU6050");
    led_setBlinkTimer(200, 100);
    return mpuProblem;  
  } else {
     	  DbgOut(DBGINFO, "I2C device detected on 0x68");    
  }


  // Set SAMPLE RATE DIVIDER
  mpu6050_beginTransmission(MPU_ADDR);
  mpu6050_write(MPU_SAMPLE_RATE_DIVIDER);
  mpu6050_write(2);
  mpuProblem =  mpu6050_endTransmissionStop();
  if (mpuProblem) {
 	  DbgOut(DBGINFO, "## could not set sample rate divider ##");
    led_setBlinkTimer(200, 100);
    return mpuProblem;
  } else {
 	  DbgOut(DBGINFO, "mpu6050 sample rate divider OK ");
  }



#ifdef ACCELGYRO_MPU6050_USE_FIFO
  // Enable FIFO
  mpu6050_beginTransmission(MPU_ADDR);
  mpu6050_write(MPU_USER_CTRL);
  // ENABLE FIFO_EN on bit 6 [7-0]
  // FIFO RESET on bit 2 [7-0]
  mpu6050_write(0x44);
  uint8_t fifoProblem =  mpu6050_endTransmissionStop();
  if (fifoProblem) {
 	  DbgOut(DBGINFO, "## mpu6050 fifo cannot enable ##");
    led_setBlinkTimer(200, 100);
    return fifoProblem;
  } else {
 	  DbgOut(DBGINFO, "mpu6050 fifo ON ");
  }

  // Enable FIFO ACC only
  mpu6050_beginTransmission(MPU_ADDR);
  mpu6050_write(MPU_FIFO);
  // ACCEL_ FIFO_EN on bit 3 [7-0]
  mpu6050_write(0x08);
  fifoProblem =  mpu6050_endTransmissionStop();
  if (fifoProblem) {
 	  DbgOut(DBGINFO, "## mpu6050 fifo cannot enable ACC ##");
    led_setBlinkTimer(200, 100);
    return fifoProblem;
  } else {
 	  DbgOut(DBGINFO, "mpu6050 acc fifo set ");
  }


#endif

  return 0;
}

uint16_t mpu6050_getFIFOCount() {
  mpu6050_beginTransmission(MPU_ADDR);
  mpu6050_write(MPU_FIFO_COUNT_H);
  mpu6050_endTransmission(0);
  mpu6050_beginTransmission(MPU_ADDR);

  uint8_t rxBuffer[2];
  twi_readFrom(MPU_ADDR, rxBuffer, 2, true);

  uint16_t fifoCount = (rxBuffer[0] << 8) + rxBuffer[1];
  return fifoCount;
}


void mpu6050_resetFifo() {
  // RESET
  mpu6050_beginTransmission(MPU_ADDR);
  mpu6050_write(MPU_USER_CTRL);
  // FIFO RESET on bit 2 [7-0]
  mpu6050_write(0x04);
  mpu6050_endTransmissionStop();
}

int cpt32 = 0;



void mpu6050_readAcc() {
    mpu6050_beginTransmission(MPU_ADDR);
    mpu6050_write(MPU_ACCEL_XOUT_H);
    mpu6050_endTransmission(0);
    mpu6050_beginTransmission(MPU_ADDR);

    uint8_t rxBuffer[4];
    uint8_t read; 
    if ((read = twi_readFrom(MPU_ADDR, rxBuffer, 4, true)) != 4) {
  	  DbgOut(DBGERROR, "Read error %d", read);
    }

    int16_t x = (rxBuffer[0] << 8) + rxBuffer[1];
    int16_t y = (rxBuffer[2] << 8) + rxBuffer[3];


    xSum_ += (x - cx_);
    ySum_ += (y - cy_);
    nSum_++;

    ax_ = x;
    ay_ = y;

    // if ((cpt32 % 500 )== 0) {
    //   DbgOut(DBGINFO, "raw :  %d\t %d ", x, y);
    //   DbgOut(DBGINFO, " ->  x  %d  /  y  %d ", xSum_, ySum_);
    // }

}


void mpu6050_ReadData(int *x, int *y) {

  if (mpuProblem != 0) {
    *x = 0;
    *y = 0;
  } else {
    cpt32++;
#ifdef ACCELGYRO_MPU6050_USE_FIFO
    flushFifo();
#else
   mpu6050_readAcc();
#endif
    get(x, y);
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



uint8_t mpu6050_readFifo(uint8_t *buf, uint8_t size) {
    mpu6050_beginTransmission(MPU_ADDR);
    mpu6050_write(MPU_FIFO_R_W);
    mpu6050_endTransmission(0);
    mpu6050_beginTransmission(MPU_ADDR);

    return twi_readFrom(MPU_ADDR, buf, size, true);
}



// ======== FROM HERE =================
// https://os.mbed.com/users/mjr/code/Pinscape_Controller_V2/


        

// accelerometer input history item, for gathering calibration data
typedef struct AccHist
{
    // reading for this entry
    int x, y;
    
    // (distance from previous entry) squared
    int32_t dsq;
    
    // total and count of samples averaged over this period
    int32_t xtot, ytot;
    int cnt;    
} AccHist_t;


#define maxAccPrv  5
struct AccHist accPrv_[maxAccPrv];



void AccHist_clearAvg(AccHist_t *cur) { 
    cur->xtot = 0;
    cur->ytot = 0; 
    cur->cnt = 0; 
}

void AccHist_addAvg(AccHist_t *cur, int x, int y) { 
    cur->xtot += x; 
    cur->ytot += y; 
    cur->cnt++; 
}

int AccHist_xAvg(AccHist_t *cur) { 
  return cur->xtot / cur->cnt; 
}

int AccHist_yAvg(AccHist_t *cur) { 
  return cur->ytot / cur->cnt; 
}

int32_t AccHist_distanceSquared(AccHist_t *p1, AccHist_t *p2) { 
    int32_t d1 = p2->x - p1->x;
    d1 *= d1;
    int32_t d2 = p2->y - p1->y; 
    d2 *= d2;
    return d1 + d2; 
}

void AccHist_set(AccHist_t *cur, int x, int y, AccHist_t *prv)
{
    // save the raw position
    cur->x = x;
    cur->y = y;
    cur->dsq = AccHist_distanceSquared(cur, prv);
}




// adjust a raw acceleration figure to a usb report value
int rawToReport(int i)
{
    // if it's near the center, scale it roughly as 20*(i/20)^2,
    // to suppress noise near the rest position
    static const int filter[] = { 
        -18, -16, -14, -13, -11, -10, -8, -7, -6, -5, -4, -3, -2, -2, -1, -1, 0, 0, 0, 0,
        0,
        0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 10, 11, 13, 14, 16, 18
    };

    // Scale to the joystick report range.  The accelerometer
    // readings use the native 14-bit signed integer representation,
    // so their scale is 2^13.
    //
    // The 1G range is special: it uses the 2G native hardware range,
    // but rescales the result to a 1G range for the joystick reports.
    // So for that mode, we divide by 4096 rather than 8192.  All of
    // the other modes map use the hardware scaling directly.
    
    // MPU6050 is on 16 bits so we divide by 4
    i /= 4;
    int smoothedValue =  (i > 20 || i < -20 ? i : filter[ i + 20]);
    smoothedValue /= 64;
    return smoothedValue;
}

// Poll the accelerometer.  Returns true on success, false if the
// device appears to be wedged (because we haven't received a unique
// sample in a long time).  The caller can try re-creating the Accel
// object if the device is wedged.
bool flushFifo()
{
    static uint8_t buffer[120];
    static int cptDebug = 0;


    // read samples until we clear the FIFO
    uint16_t fifoCount  = mpu6050_getFIFOCount();

    cptDebug++;

    uint8_t sizeToRead = fifoCount > 120 ? 120 : fifoCount;
    if (sizeToRead > 0) {
      mpu6050_readFifo(buffer, sizeToRead);
      mpu6050_resetFifo();
    }

    for (int i = 0; i < sizeToRead; i+=6) {
        // read the raw data
        int16_t x, y, z;

        x = (buffer[i] << 8) + buffer[i + 1];
        y = (buffer[i + 2] << 8) + buffer[i + 3 ];
        z = (buffer[i + 4] << 8) + buffer[i + 5];


        // if ((cptDebug % 10) == 0 && i < 18) {
        //  	DbgOut(DBGINFO, "%i : x:%d - y:%d - z:%d", i, x, y, z);
        // }

        // add the new reading to the running total for averaging
        xSum_ += (x - cx_);
        ySum_ += (y - cy_);
        nSum_++;
        
        // store the updates
        ax_ = x;
        ay_ = y;
        az_ = z;

    }
    
    return true;
}
    
void get(int *x, int * y) {
      // read the shared data and store locally for calculations
    int ax = ax_, ay = ay_;
    int xSum = xSum_, ySum = ySum_;
    int nSum = nSum_;
      
    // reset the average accumulators for the next run
    xSum_ = ySum_ = 0;
    nSum_ = 0;

    // add this sample to the current calibration interval's running total
    AccHist_t *p = &accPrv_[iAccPrv_];
    AccHist_addAvg(p, ax, ay);

    // Auto center every s so check every 0.5s as we need 5 samples
    // We check auto center every 2.5 s
    uint32_t tCenter = led_millis();
    if (tCenter > autoCenterCheckTime_ + 500)
    {
        autoCenterCheckTime_ = tCenter;
        // add the latest raw sample to the history list
        AccHist_t *prv = p;
        iAccPrv_ ++;
        // if we have a full complement, check for auto-centering
        if (iAccPrv_ >= maxAccPrv) {
            iAccPrv_ = 0;

            // Center if:
            //
            // - Auto-centering is on, and we've been stable over the
            //   whole sample period at our spot-check points
            //
            // - A manual centering request is pending
            //
            static const int32_t accTol = 164*164;  // 1% of range, squared
            AccHist_t *p0 = accPrv_;
            if (p0[0].dsq < accTol
              && p0[1].dsq < accTol
              && p0[2].dsq < accTol
              && p0[3].dsq < accTol
              && p0[4].dsq < accTol)
            {
                // Figure the new calibration point as the average of
                // the samples over the rest period
                cx_ = (AccHist_xAvg(&p0[0]) + AccHist_xAvg(&p0[1]) + AccHist_xAvg(&p0[2]) + AccHist_xAvg(&p0[3]) + AccHist_xAvg(&p0[4])) / 5;
                cy_ = (AccHist_yAvg(&p0[0]) + AccHist_yAvg(&p0[1]) + AccHist_yAvg(&p0[2]) + AccHist_yAvg(&p0[3]) + AccHist_yAvg(&p0[4])) / 5;
                // DbgOut(DBGINFO, ">>>>> New center : %d %i", cx_, cy_);s
            } else {
                // DbgOut(DBGINFO, "MOVE ==>: \t%ld \t%ld \t%ld \t%ld \t%ld ", p0[0].dsq, p0[1].dsq, p0[2].dsq, p0[3].dsq, p0[4].dsq);
            }
        }
          
        p = &accPrv_[iAccPrv_];
        AccHist_t *p0 = accPrv_;
        AccHist_set(p, ax, ay, prv);
        // clear the new item's running totals
        AccHist_clearAvg(p);
    }
      

    *x = rawToReport(xSum / nSum);
    *y = rawToReport(ySum / nSum);
    if ((cpt32 % 500) == 0) {
   	    DbgOut(DBGINFO, "mpu6050 \tX %d \tY %d ", *x, *y);
    }

}