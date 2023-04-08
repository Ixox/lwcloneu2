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

 /* Name: hid_input.c
 * Project: V-USB Mame Panel
 * Author: Andreas Oberdorfer
 * Creation Date: 2009-09-19
 * Copyright 2009 - 2011 Andreas Oberdorfer
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 */

#include <string.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#include <hwconfig.h>
#include "panel.h"
#include "comm.h"
#include "keydefs.h"
#include "clock.h"

#include "millis_timer.h"

#ifdef ACCELGYRO_MPU6050
#include "mpu6050.h"
#endif



#if !defined(PANEL_TASK)
	void panel_init(void) {}
	uint8_t panel_get_report(uint8_t **ppdata) { return 0; }
#else


const int DEBOUNCE = 5;

// derive the number of inputs from the table, let the compiler check that no pin is used twice
enum { 
	#define MAP(port, pin, normal_id, shift_id) port##pin##_index,
	PANEL_MAPPING_TABLE(MAP)
	NUMBER_OF_INPUTS,
	#undef MAP
	#if defined(LED_MAPPING_TABLE)
	#define MAP(port, pin, inv, timer_delay, pwm_compare_register) port##pin##_index,
	LED_MAPPING_TABLE(MAP)
	#undef MAP
	#endif
	#if defined(ADC_MAPPING_TABLE)
	#define MAP(port, pin, mux, minval, maxval, joyid, axis) port##pin##_index,
	ADC_MAPPING_TABLE(MAP)
	#undef MAP
	#endif
};

#if defined(ADC_MAPPING_TABLE)
enum {
	#define MAP(port, pin, mux, minval, maxval, joyid, axis) port##pin##_adcindex,
	ADC_MAPPING_TABLE(MAP)
	#undef MAP
	NUM_ADC_CHANNELS
};
#endif

static uint8_t ReportBuffer[8];
static uint8_t InputState[NUMBER_OF_INPUTS];
static uint8_t shift_key = 0;
static uint8_t shift_key_cleanup = 0;
static uint8_t need_key_update = 0;
static uint8_t need_consumer_update = 0;

#if (NUM_JOYSTICKS >= 1)
static uint8_t need_joystick_update[NUM_JOYSTICKS];
#endif

#if (USE_ACCELGYRO)
static uint8_t need_accelgyro_update = 0;
#endif

#if (USE_MOUSE != 0)
static uint8_t need_mouse_update = 0;
static uint8_t mouse_x_last_clk_state = 0;
static uint8_t mouse_x_last_dir_state = 0;
static int8_t mouse_x_count = 0;
static uint8_t mouse_y_last_clk_state = 0;
static uint8_t mouse_y_last_dir_state = 0;
static int8_t mouse_y_count = 0;
#if !defined(MOUSE_X_DELTA)
#define MOUSE_X_DELTA 1
#endif
#if !defined(MOUSE_Y_DELTA)
#define MOUSE_Y_DELTA 1
#endif
#endif


#if defined(ENABLE_ANALOG_INPUT)
static uint16_t adc_values[NUM_ADC_CHANNELS] = {0};
static const uint8_t adc_mux_table[NUM_ADC_CHANNELS] = {
	#define MAP(port, pin, mux, minval, maxval, joyid, axis) mux,
	ADC_MAPPING_TABLE(MAP)
	#undef MAP
};
#endif


// Shift switch off
PROGMEM const uint8_t NormalMapping[NUMBER_OF_INPUTS] =
{ 
	#define MAP(port, pin, normal_id, shift_id) normal_id,
	PANEL_MAPPING_TABLE(MAP)
	#undef MAP
};

// Shift switch on
PROGMEM const uint8_t ShiftMapping[NUMBER_OF_INPUTS] =
{
	#define MAP(port, pin, normal_id, shift_id) shift_id,
	PANEL_MAPPING_TABLE(MAP)
	#undef MAP
};


#define IsKeyDown(index) (InputState[index] & 0x80)

static uint8_t GetKeyNormalMap(unsigned char index) { return (index < NUMBER_OF_INPUTS) ? pgm_read_byte(NormalMapping + index) : 0; }
static uint8_t GetKeyShiftMap(unsigned char index) { return (index < NUMBER_OF_INPUTS) ? pgm_read_byte(ShiftMapping + index): 0; }
static uint8_t IsKeyboardCode(uint8_t key) { return (key >= KEY_A) && (key <= MOD_RightGUI); }
static uint8_t IsModifierCode(uint8_t key) { return (key >= MOD_LeftControl) && (key <= MOD_RightGUI); }
static uint8_t IsConsumerCode(uint8_t key) { return (key >= AC_VolumeUp) && (key <= AC_Mute); }
static uint8_t GetKey(uint8_t index) { return (shift_key != 0) ? GetKeyShiftMap(index) : GetKeyNormalMap(index); }


#if (NUM_JOYSTICKS >= 1)

static uint8_t IsJoystickCode(uint8_t key, uint8_t joy)
{
	key -= (joy * NR_OF_EVENTS_PER_JOY);

	return (key >= J1_Left) && (key < (J1_Left + NR_OF_EVENTS_PER_JOY));
}

static uint8_t NeedJoystickUpdate(void)
{
	uint8_t i;

	for (i = 0; i < NUM_JOYSTICKS; i++)
	{
		if (need_joystick_update[i]) {
			return 1;
		}
	}

	return 0;
}

#endif

#if (USE_ACCELGYRO)

static uint8_t IsAccelGyroCode(uint8_t key)
{
	key -= (4 * NR_OF_EVENTS_PER_JOY);

	return (key >= J1_Left) && (key < (J1_Left + NR_OF_EVENTS_PER_JOY));
}

#endif

#if (USE_MOUSE != 0)

static uint8_t IsMouseButtonCode(uint8_t key)
{
	return (key >= MB_Left) && (key <= MB_Middle);
}

static void MouseMoveX(uint8_t direction)
{
	if (direction)
	{
		if (mouse_x_count > -(127 - MOUSE_X_DELTA)) {
		    mouse_x_count -= MOUSE_X_DELTA;
		}
	}
	else
	{
		if (mouse_x_count < (127 - MOUSE_X_DELTA)) {
		    mouse_x_count += MOUSE_X_DELTA;
		}
	}

	need_mouse_update = 1;
}

static void MouseMoveY(uint8_t direction)
{
	if (direction)
	{
		if (mouse_y_count > -(127 - MOUSE_Y_DELTA)) {
		    mouse_y_count -= MOUSE_Y_DELTA;
		}
	}
	else
	{
		if (mouse_y_count < (127 - MOUSE_Y_DELTA)) {
		    mouse_y_count += MOUSE_Y_DELTA;
		}
	}

	need_mouse_update = 1;
}

static void CheckMouseUpdate(void)
{
	#if defined(MOUSE_X_CLK_INDEX) && defined(MOUSE_X_DIR_INDEX)

	uint8_t mouse_clk_state = InputState[MOUSE_X_CLK_INDEX];
	uint8_t mouse_dir_state = InputState[MOUSE_X_DIR_INDEX];

	if (mouse_clk_state != mouse_x_last_clk_state)
	{
		if (mouse_dir_state == mouse_x_last_dir_state) {
			MouseMoveX(mouse_clk_state ^ mouse_dir_state);
		}

		mouse_x_last_clk_state = mouse_clk_state;
	}

	if (mouse_dir_state != mouse_x_last_dir_state)
	{
		if (mouse_clk_state == mouse_x_last_clk_state) {
			MouseMoveX(!(mouse_clk_state ^ mouse_dir_state));
		}

		mouse_x_last_dir_state = mouse_dir_state;
	}

	#endif

	#if defined(MOUSE_Y_CLK_INDEX) && defined(MOUSE_Y_DIR_INDEX)

	mouse_clk_state = InputState[MOUSE_Y_CLK_INDEX];
	mouse_dir_state = InputState[MOUSE_Y_DIR_INDEX];

	if (mouse_clk_state != mouse_y_last_clk_state)
	{
		if (mouse_dir_state == mouse_y_last_dir_state) {
			MouseMoveY(mouse_clk_state ^ mouse_dir_state);
		}

		mouse_y_last_clk_state = mouse_clk_state;
	}

	if (mouse_dir_state != mouse_y_last_dir_state)
	{
		if (mouse_clk_state == mouse_y_last_clk_state) {
			MouseMoveY(!(mouse_clk_state ^ mouse_dir_state));
		}

		mouse_y_last_dir_state = mouse_dir_state;
	}

	#endif
}

static uint8_t NeedMouseUpdate(void) { return need_mouse_update; }

#endif

void panel_init(void)
{
	millis_init();

	#ifdef ACCELGYRO_MPU6050
	mpu6050_init();
	#endif

	#if (NUM_JOYSTICKS >= 1)
	memset(&need_joystick_update[0], 0x00, sizeof(need_joystick_update));
	#endif

	#define MAP(port, pin, normal_id, shift_id) \
		PORT##port |= (1 << pin); \
		DDR##port &= ~(1 << pin);
	PANEL_MAPPING_TABLE(MAP)
	#undef MAP

	#if defined(ENABLE_ANALOG_INPUT) && defined(ADC_MAPPING_TABLE)
	#define MAP(port, pin, mux, minval, maxval, joyid, axis) \
		PORT##port &= ~(1 << pin); \
		DDR##port &= ~(1 << pin);
	ADC_MAPPING_TABLE(MAP)
	#undef MAP

	ADC_init();
	#endif


};

static void SetNeedUpdate(uint8_t index)
{
	uint8_t key = GetKey(index);

	if (IsConsumerCode(key))
	{
		need_consumer_update = 1;
		return;
	}

	if (IsKeyboardCode(key))
	{
		need_key_update = 1;
		return;
	}

	#if (USE_MOUSE != 0)
	if (IsMouseButtonCode(key))
	{
		need_mouse_update = 1;
		return;
	}
	#endif

	#if (NUM_JOYSTICKS >= 1)
	{
		uint8_t i;

		for (i = 0; i < NUM_JOYSTICKS; i++)
		{
			if (IsJoystickCode(key, i))
			{
				need_joystick_update[i] = 1;
				return;
			}
		}
	}
	#endif

	#if (USE_ACCELGYRO)
	if (IsAccelGyroCode(key))
	{
		need_accelgyro_update = 1;
		return;
	}
	#endif
}

#if defined(SHIFT_SWITCH_INDEX)

static void ShiftKeyCleanUp(void)
{
	if (shift_key_cleanup == 1)
	{
		uint8_t i;
		shift_key_cleanup = 2;

		for (i = 0; i < NUMBER_OF_INPUTS; i++)
		{
			if (i != SHIFT_SWITCH_INDEX)
			{
				if (InputState[i] != 0)
				{
					if (GetKeyNormalMap(i) != GetKeyShiftMap(i))
					{
						SetNeedUpdate(i);
						InputState[i] = 0;
					}
				}
			}
		}
	}

	if (shift_key_cleanup == 2)
	{
		bool no_update = !need_consumer_update && !need_key_update;

		#if (USE_MOUSE != 0)
		no_update = no_update && !NeedMouseUpdate();
		#endif

		#if (NUM_JOYSTICKS >= 1)
		no_update = no_update && !NeedJoystickUpdate();
		#endif

		#if (USE_ACCELGYRO)
		no_update = no_update && !need_accelgyro_update;
		#endif

		if (no_update)
		{
			shift_key_cleanup = 0;
			shift_key = IsKeyDown(SHIFT_SWITCH_INDEX);
		}
	}
}

#endif

static uint8_t NeedUpdate(void)
{
	#if defined(SHIFT_SWITCH_INDEX)
	ShiftKeyCleanUp();
	#endif

	#if (USE_MOUSE != 0)
	if (NeedMouseUpdate())
	{
		need_mouse_update = 0;
		return ID_Mouse;
	}
	#endif

	if (need_key_update)
	{
		need_key_update = 0;
		return ID_Keyboard;
	}

	if (need_consumer_update)
	{
		need_consumer_update = 0;
		return ID_Consumer;
	}

	uint8_t analog_update[NUM_JOYSTICKS + 1] = {0};
	static uint8_t analog_counter[NUM_JOYSTICKS + 1];

	#if (NUM_JOYSTICKS >= 1)
	for (uint8_t i = 0; i < NUM_JOYSTICKS; i++)
	{
		if (analog_counter[i] < 0xFF && need_joystick_update[i])
		    analog_counter[i] += 1;
	}
	#endif

	#if (USE_ACCELGYRO)
	if (analog_counter[NUM_JOYSTICKS] < 0xFF && need_accelgyro_update)
		analog_counter[NUM_JOYSTICKS] += 1;
	#endif

	#if defined(ENABLE_ANALOG_INPUT) && defined(ADC_MAPPING_TABLE)

	#define MAP(port, pin, mux, minval, maxval, joyid, axis) \
	if (joyid >= ID_Joystick1 && (joyid - ID_Joystick1) < NUM_JOYSTICKS) { analog_update[joyid - ID_Joystick1] = 1; } else \
	if (joyid == ID_AccelGyro) { analog_update[NUM_JOYSTICKS] = 1; }
	ADC_MAPPING_TABLE(MAP)
	#undef MAP

	for (uint8_t i = 0; i < sizeof(analog_update) / sizeof(analog_update[0]); i++)
	{
		if (analog_counter[i] < 0xFF && analog_update[i] > 0)
		    analog_counter[i] += 1;
	}

	#endif

	uint8_t ac_max = 0;
	int8_t index_max = -1;

	for (int8_t i = 0; i < sizeof(analog_update) / sizeof(analog_update[0]); i++)
	{
		if (ac_max < analog_counter[i])
		{
			ac_max = analog_counter[i];
			index_max = i;
		}
	}

	if (index_max >= 0)
	{
		analog_counter[index_max] = 0;

		#if (NUM_JOYSTICKS >= 1)
		if (index_max < NUM_JOYSTICKS) {
			need_joystick_update[index_max] = 0;
			return index_max + ID_Joystick1;
		}
		#endif

		#if (USE_ACCELGYRO)
		need_accelgyro_update = 0;
		return ID_AccelGyro;
		#endif
	}

	return ID_Unknown;
}

static void SetInputCount(uint8_t index, uint8_t condition)
{
	#if (USE_MOUSE != 0) && defined(MOUSE_X_CLK_INDEX) && defined(MOUSE_X_DIR_INDEX)
	if ((index == MOUSE_X_CLK_INDEX) || (index == MOUSE_X_DIR_INDEX))
	{
		InputState[index] = condition;
		return;
	}
	#endif

	#if (USE_MOUSE != 0) && defined(MOUSE_Y_CLK_INDEX) && defined(MOUSE_Y_DIR_INDEX)
	if ((index == MOUSE_Y_CLK_INDEX) || (index == MOUSE_Y_DIR_INDEX))
	{
		InputState[index] = condition;
		return;
	}
	#endif

	if (index >= NUMBER_OF_INPUTS)
	{
		return;
	}

	#if defined(MULTIFIRE_INDEX)
	if (index == MULTIFIRE_INDEX)
	{
		static uint16_t ncycle = 0;
		static uint8_t ncount = 0;
		static uint8_t ndelay = 0;

		// simple debounce
		if (ndelay == 0 && condition)
		{
			ndelay = (100 / (DELTA_TIME_PANEL_REPORT_MS + 1));
			ncount += MULTIFIRE_COUNT;
		}
		else if (ndelay > 1)
		{
			ndelay--;
		}
		else if (ndelay == 1)
		{
			if (!condition)
				ndelay = 0;
		}

		condition = 0;

		// state machine to generate multiple events
		if (ncount > 0)
		{
			condition = (ncycle < (100 / (DELTA_TIME_PANEL_REPORT_MS + 1))) ? 1 : 0;

			if (ncycle >= (600 / (DELTA_TIME_PANEL_REPORT_MS + 1)))
			{
				ncycle = 0;
				ncount -= 1;
			}
			else
			{
				ncycle += 1;
			}
		}
	}
	#endif

	uint8_t changed = 0;
	uint8_t count = InputState[index];
	uint8_t state = count & 0x80;
	count &= 0x7f;

	if (condition)
	{
		if (count <= DEBOUNCE)
		{
			if ((count == DEBOUNCE) && !state)
			{
				changed = 1;
				state = 0x80;
			}

			count++;
		}
		else
		{
			return;
		}
	}
	else
	{
		if (count > 0)
		{
			if ((count == 1) && state)
			{
				changed = 1;
				state = 0;
			}

			count--;
		}
		else
		{
			return;
		}
	}

	InputState[index] = state | count;

	if (changed)
	{
		#if defined(SHIFT_SWITCH_INDEX)
		if (index == SHIFT_SWITCH_INDEX)
		{
			shift_key_cleanup = 1;
		}
		else
		#endif
		{
			SetNeedUpdate(index);
		}
	}
}

void panel_ScanInput(void)
{
	if (shift_key_cleanup) {
		return;
	}

	#define MAP(port, pin, normal_id, shift_id) SetInputCount(port##pin##_index, 0 == (PIN##port & (1 << pin)));
	PANEL_MAPPING_TABLE(MAP)
	#undef MAP

	#if (USE_MOUSE != 0)
	CheckMouseUpdate();
	#endif
}

#if (USE_CONSUMER != 0)
static uint8_t ReportConsumer(void)
{
	uint8_t i;
	uint8_t consumer = 0;

	for (i = 0; i < NUMBER_OF_INPUTS; i++)
	{
		if (IsKeyDown(i))
		{
			uint8_t key = GetKey(i);

			if (IsConsumerCode(key)) {
				consumer |= ConsumerBit(key);
			}
		}
	}

	ReportBuffer[0] = ID_Consumer;
	ReportBuffer[1] = consumer;

	return 2;
}
#endif

static uint8_t ReportKeyboard(void)
{
	uint8_t i;
	uint8_t r = 2;

	memset(&ReportBuffer[1], 0x00, sizeof(ReportBuffer) - 1);

	ReportBuffer[0] = ID_Keyboard;

	for (i = 0; i < NUMBER_OF_INPUTS; i++)
	{
		if (IsKeyDown(i))
		{
			uint8_t key = GetKey(i);

			if (IsKeyboardCode(key))
			{
				if (IsModifierCode(key))
				{
					ReportBuffer[1] |= ModifierBit(key);
				}
				else
				{
					if (r < sizeof(ReportBuffer))
					{
						switch (key)
						{
						case KM_ALT_F4:
							ReportBuffer[1] |= ModifierBit(MOD_LeftAlt);
							ReportBuffer[r] = KEY_F4;
							break;
						case KM_SHIFT_F7:
							ReportBuffer[1] |= ModifierBit(MOD_LeftShift);
							ReportBuffer[r] = KEY_F7;
							break;
						default:
							ReportBuffer[r] = key;
							break;
						}

						r++;
					}
				}
			}
		}
	}

	return sizeof(ReportBuffer);
}

#if defined(ENABLE_ANALOG_INPUT)

uint16_t ADC_getvalue(uint8_t id)
{
	uint16_t x;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		x = adc_values[id];
	}

	return x;
}

static int16_t joyval12(uint16_t x, int16_t minval, int16_t maxval)
{
	return (int16_t)(((int32_t)x * (int32_t)(maxval - minval) + (1 << 9)) >> 10) + minval - 2047;
}

static int8_t joyval8(uint16_t x, int16_t minval, int16_t maxval)
{
	return (int8_t)(((int32_t)x * (int32_t)(maxval - minval) + (1 << 9)) >> 10) + minval - 127;
}

#endif

#if (NUM_JOYSTICKS >= 1)

static uint8_t ReportJoystick(uint8_t id)
{
	uint8_t i;
	int16_t joy_x = 0;
	int16_t joy_y = 0;
	uint8_t joy_b = 0;

	ReportBuffer[0] = id;

	#if defined(ENABLE_ANALOG_INPUT) && defined(ADC_MAPPING_TABLE)
	#define MAP(port, pin, mux, minval, maxval, joyid, axis) \
		if ((axis == 0) && (joyid == id)) { joy_x = joyval12(ADC_getvalue(port##pin##_adcindex), (int16_t)(minval * 4094), (int16_t)(maxval * 4094)); } \
		if ((axis == 1) && (joyid == id)) { joy_y = joyval12(ADC_getvalue(port##pin##_adcindex), (int16_t)(minval * 4094), (int16_t)(maxval * 4094)); }
	ADC_MAPPING_TABLE(MAP)
	#undef MAP
	#endif

	id = (id - ID_Joystick1) * NR_OF_EVENTS_PER_JOY;

	for (i = 0; i < NUMBER_OF_INPUTS; i++)
	{
		if (IsKeyDown(i))
		{
			uint8_t key = GetKey(i) - id;
			switch (key)
			{
			case J1_Left:
				joy_x = -2047;
				break;
			case J1_Right:
				joy_x = +2047;
				break;
			case J1_Up:
				joy_y = -2047;
				break;
			case J1_Down:
				joy_y = +2047;
				break;

			case J1_Button1:
			case J1_Button2:
			case J1_Button3:
			case J1_Button4:
			case J1_Button5:
			case J1_Button6:
			case J1_Button7:
			case J1_Button8:
				joy_b |= JoyButtonBit(key);
				break;

			default:
				break;
			}
		}
	}

	ReportBuffer[1] = ((uint16_t)joy_x & 0xFF);
	ReportBuffer[2] = (((uint16_t)joy_y & 0x0F) << 4) | (((uint16_t)joy_x >> 8) & 0x0F);
	ReportBuffer[3] = (((uint16_t)joy_y >> 4) & 0xFF);
	ReportBuffer[4] = joy_b;

	return 5;
}

#endif

#if (USE_ACCELGYRO)

#ifndef ACCELGYRO_MPU6050

static uint8_t ReportAccelGyro()
{
	uint8_t id = ID_AccelGyro;

	int8_t joy_x = 0;
	int8_t joy_y = 0;
	int8_t joy_z = 0;
	int8_t joy_rx = 0;
	int8_t joy_ry = 0;
	int8_t joy_rz = 0;
	uint8_t joy_b = 0;

	ReportBuffer[0] = id;

	#if defined(ENABLE_ANALOG_INPUT) && defined(ADC_MAPPING_TABLE)
	#define MAP(port, pin, mux, minval, maxval, joyid, axis) \
		if ((axis == 0) && (joyid == id)) { joy_x  = joyval8(ADC_getvalue(port##pin##_adcindex), (int16_t)(minval * 254), (int16_t)(maxval * 254)); } \
		if ((axis == 1) && (joyid == id)) { joy_y  = joyval8(ADC_getvalue(port##pin##_adcindex), (int16_t)(minval * 254), (int16_t)(maxval * 254)); } \
		if ((axis == 2) && (joyid == id)) { joy_z  = joyval8(ADC_getvalue(port##pin##_adcindex), (int16_t)(minval * 254), (int16_t)(maxval * 254)); } \
		if ((axis == 3) && (joyid == id)) { joy_rx = joyval8(ADC_getvalue(port##pin##_adcindex), (int16_t)(minval * 254), (int16_t)(maxval * 254)); } \
		if ((axis == 4) && (joyid == id)) { joy_ry = joyval8(ADC_getvalue(port##pin##_adcindex), (int16_t)(minval * 254), (int16_t)(maxval * 254)); } \
		if ((axis == 5) && (joyid == id)) { joy_rz = joyval8(ADC_getvalue(port##pin##_adcindex), (int16_t)(minval * 254), (int16_t)(maxval * 254)); }
	ADC_MAPPING_TABLE(MAP)
	#undef MAP
	#endif

	id = (id - ID_Joystick1) * NR_OF_EVENTS_PER_JOY;

	for (uint8_t i = 0; i < NUMBER_OF_INPUTS; i++)
	{
		if (IsKeyDown(i))
		{
			uint8_t key = GetKey(i) - id;
			switch (key)
			{
			case J1_Left:
				joy_x = -127;
				break;
			case J1_Right:
				joy_x = +127;
				break;
			case J1_Up:
				joy_y = -127;
				break;
			case J1_Down:
				joy_y = +127;
				break;

			case J1_Button1:
			case J1_Button2:
			case J1_Button3:
			case J1_Button4:
			case J1_Button5:
			case J1_Button6:
			case J1_Button7:
			case J1_Button8:
				joy_b |= JoyButtonBit(key);
				break;

			default:
				break;
			}
		}
	}

	ReportBuffer[1] = joy_x;
	ReportBuffer[2] = joy_y;
	ReportBuffer[3] = joy_z;
	ReportBuffer[4] = joy_rx;
	ReportBuffer[5] = joy_ry;
	ReportBuffer[6] = joy_rz;
	ReportBuffer[7] = joy_b;

	return 8;
}
#endif

#ifdef ACCELGYRO_MPU6050

static uint8_t ReportAccelGyro()
{

	int8_t acc_x = 0;
	int8_t acc_y = 0;
	int8_t plunger_z = 0;


	#if defined(ENABLE_ANALOG_INPUT) && defined(ADC_MAPPING_TABLE)
	#define MAP(port, pin, mux, minval, maxval, joyid, axis) \
		if ((axis == 2) && (joyid == ID_AccelGyro)) { 	plunger_z  = joyval8(ADC_getvalue(port##pin##_adcindex), (int16_t)(minval * 254), (int16_t)(maxval * 254)); }
	ADC_MAPPING_TABLE(MAP)
	#undef MAP
	#endif

	ReportBuffer[0] = ID_AccelGyro;
	
	int x = 0, y = 0;
	mpu6050_ReadData(&x, &y);

	ReportBuffer[1] = x;
	ReportBuffer[2] = y;
	ReportBuffer[3] = plunger_z;
	ReportBuffer[4] = 0;
	ReportBuffer[5] = 0;
	ReportBuffer[6] = 0;
	ReportBuffer[7] = 0;

	return 8;
}
#endif



#endif


#if (USE_MOUSE != 0)

static uint8_t ReportMouse(void)
{
	uint8_t i;
	uint8_t buttons = 0;

	for (i = 0; i < NUMBER_OF_INPUTS; i++)
	{
		if (IsKeyDown(i))
		{
			uint8_t key = GetKey(i);

			if (IsMouseButtonCode(key)) {
				buttons |= MouseButtonBit(key);
			}
		}
	}

	ReportBuffer[0] = ID_Mouse;
	ReportBuffer[1] = buttons;
	ReportBuffer[2] = mouse_x_count;
	ReportBuffer[3] = mouse_y_count;
	mouse_x_count = 0;
	mouse_y_count = 0;

	return 4;
}

#endif


static uint8_t BuildReport(uint8_t id)
{
	switch (id)
	{
	#if (USE_KEYBOARD != 0)
	case ID_Keyboard:
		return ReportKeyboard();
	#endif

	#if (USE_CONSUMER != 0)
	case ID_Consumer:
		return ReportConsumer();
	#endif

	#if (NUM_JOYSTICKS >= 1)
	case ID_Joystick4:
	case ID_Joystick3:
	case ID_Joystick2:
	case ID_Joystick1:
		return ReportJoystick(id);
	#endif

	#if (USE_ACCELGYRO)
	case ID_AccelGyro:
		return ReportAccelGyro(id);
	#endif

	#if (USE_MOUSE != 0)
	case ID_Mouse:
		return ReportMouse();
	#endif

	default:
		break;
	}

	return 0;
}

uint8_t panel_get_report(uint8_t **ppdata)
{
	if (ppdata == NULL) {
		return 0;
	}

	static uint16_t time_next_ms = 0;
	uint16_t const time_curr_ms = clock_ms();

	if (((int16_t)time_curr_ms - (int16_t)time_next_ms) < 0) {
		return 0;
	}

	time_next_ms = time_curr_ms + DELTA_TIME_PANEL_REPORT_MS;

	panel_ScanInput();

	uint8_t const id = NeedUpdate();

	if (id == ID_Unknown) {
		return 0;
	}

	uint8_t const ndata = BuildReport(id);
	*ppdata = ReportBuffer;

	return ndata;
}


#if defined(ENABLE_ANALOG_INPUT)

// ADC Interrupt Routine

ISR(ADC_vect)
{
	#if defined(ENABLE_PROFILING)
	profile_start();
	#endif

	static int i = 0;

	// get value

	adc_values[i] = ADC;

	// cycle

	i -= 1;

	if (i < 0) 
		i += NUM_ADC_CHANNELS;

	// set mux channel for the next conversion

	ADC_setmux(adc_mux_table[i]);

	// start new conversion

	ADCSRA |= (1 << ADSC);
}

#endif



#endif
