/*
 * LWCloneU2
 * Copyright (C) 2013 Andreas Dittrich <lwcloneu2@cithraidt.de>
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

#ifndef DEVCONFIG_H__INCLUDED
#define DEVCONFIG_H__INCLUDED

// #define DEBUGLEVEL 2


/****************************************
 USB device config
****************************************/

#define ENABLE_LED_DEVICE
#define ENABLE_ANALOG_INPUT

#define ENABLE_PANEL_DEVICE

// See keydefs.h : KEY_M = 0x10
// Mutable toys are the "leds" with a delay
#define KEY_MUTE_TOYS 0x10

#define NUM_JOYSTICKS 0
#define USE_MOUSE 0
#define USE_CONSUMER 1
#define USE_KEYBOARD 1

#define USE_ACCELGYRO 1
#define ACCELGYRO_MPU6050 1
// Does not work
// #define ACCELGYRO_MPU6050_USE_FIFO 1

#endif

