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
/* 
    mega 2560 pinout 
    https://ouilogique.com/files/2015-05-28-pinouts/images/arduino_mega_2560.png
*/

#define LED_MAPPING_TABLE(_map_) \
	\
	_map_( H, 3, 0, 20, A) /* PH3 ( OC4A )	Digital pin 6 (PWM) */ \
	_map_( H, 4, 0, 20, B) /* PH4 ( OC4B )	Digital pin 7 (PWM) */ \
	_map_( B, 4, 0, 0, 0) /*           Digital pin 10 */  \
	_map_( B, 5, 0, 0, 0) /*           Digital pin 11 */  \
	_map_( B, 6, 0, 0, 0) /*           Digital pin 12 */ \
	_map_( E, 5, 0, 0, 0) /*           Digital pin 3 */ \
	_map_( H, 6, 0, 0, 0) /*           Digital pin 9 */ \
	_map_( E, 3, 0, 0, 0) /*           Digital pin 5 */
	\
	/* end */




#if (USE_MOUSE)
#define MOUSE_X_CLK_INDEX    9
#define MOUSE_X_DIR_INDEX   10
#define MOUSE_Y_CLK_INDEX   11
#define MOUSE_Y_DIR_INDEX   12
#endif

#define SHIFT_SWITCH_INDEX   10

#define PANEL_MAPPING_TABLE(_map_) \
	\
	_map_( F, 0,    MOD_LeftShift,      AC_VolumeDown      				)  \
	_map_( F, 1,    MOD_RightShift,     AC_VolumeUp        			)  \
	_map_( F, 2,    MOD_LeftControl,    AC_Mute      	)  \
	_map_( F, 3,    MOD_RightControl,   KEY_M       	)  \
	_map_( F, 4,    AG_Button1,              AG_Button5  				)  \
	_map_( F, 5,    AG_Button2,              AG_Button6  				)  \
	_map_( F, 6,    KEY_Esc,        	0   				)  \
	_map_( F, 7,    KEY_Enter,        	0   				)  \
	_map_( L, 0,    AG_Button3,        		AG_Button7   			)  \
	_map_( L, 2,    AG_Button4,        		AG_Button8   			)  \
	_map_( L, 4,    KEY_E,        	    0   		    )  \
	\
	/* end */

// (port, pin, mux, value_min, value_max, joyid, axis
// for mega2560, mux[5:0] is 0x00..0x07 => (ADC0..ADC7) and 0x20..0x27 => (ADC8..ADC15)
// see https://ww1.microchip.com/downloads/en/devicedoc/atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf
// Page 282
// We'll use analog pin15 for the plunger linear resistor
// It will be reported as Joystick Z axis (X and Y will be accelerometer)

// Default is 0.0 / 1.0: depend on installation
// To verify with debuging or with Game controller properties in Windows
// The goal is to have a full scale fo the Z axis from one position of the plunger to the oposite position
#define PLUNGER_MIN -1.5
#define PLUNGER_MAX 1.88

#define ADC_MAPPING_TABLE(_map_) \
	\
	_map_( K, 7, 0x27, PLUNGER_MIN, PLUNGER_MAX, ID_AccelGyro, 2 ) /* Analog Pin 15 Z   */ \
	\
	/* end */
