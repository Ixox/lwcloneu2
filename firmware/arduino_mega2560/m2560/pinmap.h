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

//	_map_( B, 7, 0 ) /* ( D13 )               Digital pin 13 - led */ \


/*
 * 4th param is a timer delay after which pwm will be activate dot turn down currant. Scale is 200uS. So 1000 = 200ms
 * 5th param is the PWM value (0-49) to use to slow down currant
 */

#define LED_MAPPING_TABLE(_map_) \
	\
	_map_( A, 0, 0, 1000, 25 ) /* ( AD0 )               Digital pin 22 */ \
	_map_( A, 1, 0, 1000, 25 ) /* ( AD1 )               Digital pin 23 */ \
	_map_( A, 2, 0, 0, 0 ) /* ( AD2 )               Digital pin 24 */ \
	_map_( A, 3, 0, 0, 0 ) /* ( AD3 )               Digital pin 25 */ \
	_map_( A, 4, 0, 0, 0 ) /* ( AD4 )               Digital pin 26 */ \
	_map_( A, 5, 0, 0, 0 ) /* ( AD5 )               Digital pin 27 */ \
	_map_( A, 6, 0, 0, 0 ) /* ( AD6 )               Digital pin 28 */ \
	_map_( A, 7, 0, 0, 0 ) /* ( AD7 )               Digital pin 29 */ \
	\
	/* end */

#if (USE_MOUSE)
#define MOUSE_X_CLK_INDEX    9
#define MOUSE_X_DIR_INDEX   10
#define MOUSE_Y_CLK_INDEX   11
#define MOUSE_Y_DIR_INDEX   12
#endif

#define SHIFT_SWITCH_INDEX   13

#define PANEL_MAPPING_TABLE(_map_) \
	\
	_map_( E, 4,    MOD_LeftShift,   0                 ) /* ( OC3B/INT4 )         Digital pin 2 (PWM) */ \
	_map_( E, 5,    MOD_RightShift,  0                 ) /* ( OC3C/INT5 )         Digital pin 3 (PWM) */ \
	_map_( G, 5,    MOD_LeftControl, 0                 ) /* ( OC0B )              Digital pin 4 (PWM) */ \
	_map_( E, 3,    MOD_RightControl,0                 ) /* ( OC3A/AIN1 )         Digital pin 5 (PWM) */ \
	_map_( H, 3,    KEY_Esc,         0                 ) /* ( OC4A )              Digital pin 6 (PWM) */ \
	_map_( H, 4,    KEY_Enter,       0                 ) /* ( OC4B )              Digital pin 7 (PWM) */ \
	_map_( H, 5,    KEY_1,           KEY_P             ) /* ( OC4C )              Digital pin 8 (PWM) */ \
	_map_( H, 6,    KEY_5,           KEY_5             ) /* ( OC2B )              Digital pin 9 (PWM) */ \
	_map_( B, 4,    KEY_A,           KEY_A             ) /* ( OC2A/PCINT4 )       Digital pin 10 (PWM) */ \
	_map_( B, 5,    KEY_S,           KEY_S             ) /* ( OC1A/PCINT5 )       Digital pin 11 (PWM) */ \
	_map_( B, 6,    KEY_D,           KEY_D             ) /* ( OC1B/PCINT6 )       Digital pin 12 (PWM) */ \
	_map_( K, 4,    KEY_2,           AC_Mute           ) /* ( ADC12/PCINT20 )     Analog pin 12 */ \
	\
	/* end */


	// (port, pin, mux, value_min, value_max, joyid, axis
	// for mega2560, mux is 0x00..0x07 => (ADC0..ADC7) and 0x10..0x17 => (ADC8..ADC15)
	// We'll use analog pin2 for the plunger linear resistor
	// It will be reported as Joystick Z axis (X and Y will be accelerometer)
#define ADC_MAPPING_TABLE(_map_) \
	\
	_map_( F, 2, 0x02, 0.000, 1.000, ID_AccelGyro, 2 ) /* Analog Pin 2 Z   */ \
	\
	/* end */
