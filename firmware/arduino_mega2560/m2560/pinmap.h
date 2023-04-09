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


/*

	_map_( B, 7, 0 )
	_map_( A, 5, 0, 0, 0 ) 
	_map_( A, 6, 0, 0, 0 ) 
	_map_( A, 7, 0, 0, 0 ) 
*/

/*
 * 4th param is a timer delay after which pwm will be activate dot turn down currant. Scale is 10ms. So 20 = 200ms
 * Dedicate to Toys, do not use with PWM for led
 */


//	_map_( A, 0, 0, 20) /* ( AD0 )              Digital pin 22 */ 
//	_map_( A, 1, 0, 20 ) /* ( AD1 )             Digital pin 23 */ 

#define LED_MAPPING_TABLE(_map_) \
	\
	_map_( H, 3, 0, 20, A) /* PH3 ( OC4A )	Digital pin 6 (PWM) */ \
	_map_( H, 4, 0, 20, B) /* PH4 ( OC4B )	Digital pin 7 (PWM) */ \
	_map_( A, 0, 0, 0, 0) /* ( AD0 )              Digital pin 22 */  \
	_map_( A, 1, 0, 0, 0) /* ( AD1 )             Digital pin 23 */  \
	_map_( A, 2, 0, 0, 0) /* ( AD2 )               Digital pin 24 */ \
	\
	/* end */

#if (USE_MOUSE)
#define MOUSE_X_CLK_INDEX    9
#define MOUSE_X_DIR_INDEX   10
#define MOUSE_Y_CLK_INDEX   11
#define MOUSE_Y_DIR_INDEX   12
#endif

#define SHIFT_SWITCH_INDEX   13


//	_map_( E, 4,    MOD_LeftShift,   0                 ) /* ( OC3B/INT4 )         Digital pin 2 (PWM) */ 
//	_map_( E, 5,    MOD_RightShift,  0                 ) /* ( OC3C/INT5 )         Digital pin 3 (PWM) */ 
//	_map_( H, 3,    KEY_Esc,         0                 ) /* ( OC4A )              Digital pin 6 (PWM) */ 
//	_map_( H, 4,    KEY_Enter,       0                 ) /* ( OC4B )              Digital pin 7 (PWM) */ 
//	_map_( H, 5,    KEY_1,           KEY_P             ) /* ( OC4C )              Digital pin 8 (PWM) */ 


#define PANEL_MAPPING_TABLE(_map_) \
	\
	_map_( F, 0,    MOD_LeftShift,  0   /* A0 */         )  \
	_map_( F, 1,    MOD_RightShift, 0   /* A1 */         )  \
	_map_( F, 2,    KEY_5,            0   /* A2 */         )  \
	_map_( F, 3,    KEY_1,            0   /* A3 */         )  \
	_map_( F, 4,    KEY_Enter,        0   /* A4 */         )  \
	_map_( F, 5,    KEY_A,            0   /* A5 */         )  \
	_map_( F, 6,    KEY_B,            0   /* A6 */         )  \
	_map_( F, 7,    KEY_C,            0   /* A7 */         )  \
	_map_( K, 0,    KEY_D,            0   /* A8 */         )  \
	_map_( K, 1,    KEY_E,            0   /* A9 */         )  \
	_map_( K, 2,    KEY_F,            0   /* A10 */        )  \
	\
	/* end */


	// (port, pin, mux, value_min, value_max, joyid, axis
	// for mega2560, mux is 0x00..0x07 => (ADC0..ADC7) and 0x10..0x17 => (ADC8..ADC15)
	// We'll use analog pin2 for the plunger linear resistor
	// It will be reported as Joystick Z axis (X and Y will be accelerometer)
#define ADC_MAPPING_TABLE(_map_) \
	\
	_map_( K, 7, 0x02, 0.000, 1.000, ID_AccelGyro, 2 ) /* Analog Pin 2 Z   */ \
	\
	/* end */
