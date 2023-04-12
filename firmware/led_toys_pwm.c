/*
 * LWCloneU2
 * Copyright (C) 2013 Andreas Dittrich <lwcloneu2@cithraidt.de>
 * Modified by Xavier Hosxe to user real PWM for pins 6,7,8 (Allow PWM for solenoid power saving)
 * + Silence mode
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
 
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <hwconfig.h>
#include "led.h"
#include "comm.h"

#if !defined(LED_TIMER_vect)
	void led_init(void) {}
	void led_update(uint8_t *p8bytes) {}
#else

// Defined in panel.c
#ifdef KEY_MUTE_TOYS
extern uint8_t muteToys;
#endif

// OCCR40 to allow OOCR40 bellow. When replacing comparator by 0
int OCR40;
uint8_t timer4Used = 0;

// Real PWM is a 512 counter
// PWM is 16Khz : there's some latency when goes down, specially with a diode on the solenoid
// 250 is closer to 400/512

#ifndef PWM_POWER_MAX
#define PWM_POWER_MAX 280
#endif
#ifndef PWM_POWER_SAVING
#define PWM_POWER_SAVING 100
#endif

#define MAP(X, pin, inv, pwm_delay, pwm_compare_register) X##pin##_index,
enum { LED_MAPPING_TABLE(MAP) NUMBER_OF_LEDS };
#undef MAP

#define MAP(X, pin, inv, pwm_delay, pwm_compare_register) X##pin##_pwm_delay = pwm_delay,
enum { LED_MAPPING_TABLE(MAP)  };
#undef MAP


#define NUMBER_OF_BANKS   ((NUMBER_OF_LEDS + 7) / 8)
#define MAX_PWM 49


#if (NUMBER_OF_LEDS > 32)
	#error "number of led pins is bigger than 32!"
#endif


struct {
	volatile uint8_t enable;
	volatile uint8_t mode;
	char pwm_comparator;
} g_LED[NUMBER_OF_BANKS * 8];

// Xavier Hosxe :
// Counter to turn down the current after some time
// Avoid the toys / Solenoid to burn
uint16_t ToyTimer[NUMBER_OF_BANKS * 8];

volatile uint16_t g_dt = 256;  // access is not atomic, but the read in the pwm loop is not critical

uint16_t millis_ledTimersize = 1600;
uint16_t millis_ledTimerOffAt = 800;

volatile unsigned long millis_timer;

unsigned long led_millis(void) {
	return millis_timer;
}
void led_setBlinkTimer(uint16_t size, uint16_t offAt) {
	millis_ledTimersize = size;
    millis_ledTimerOffAt = offAt;
}

static void update_state(uint8_t * p5bytes);
static void update_profile(int8_t k, uint8_t * p8bytes);
static void update_pwm(uint8_t *pwm, int8_t n, uint16_t t);
static void led_ports_init(void);



void led_init(void)
{

	/* LED driver */
	led_ports_init();

	// Timer for soft-PWM
	led_timer_init();


	if (timer4Used) {
		// Store pwm_delay
		#define CHAR0 '0'
		#define CHARA 'A'
		#define CHARB 'B'
		#define CHARC 'C'
		#define MAP(X, pin, inv, pwm_delay, pwm_compare_register) \
		g_LED[X##pin##_index].pwm_comparator = CHAR##pwm_compare_register;
		LED_MAPPING_TABLE(MAP)
		#undef MAP
		#undef CHAR0
		#undef CHARA
		#undef CHARB
		#undef CHARC
	}

}


void led_update(uint8_t *p8bytes)
{
	static uint8_t nbank = 0;

	if (p8bytes[0] == 64)
	{
		update_state(p8bytes + 1);
		nbank = 0;
	}
	else
	{
		update_profile(nbank, p8bytes);
		nbank = (nbank + 1) & 0x03;
	}
}


static void update_state(uint8_t * p5bytes)
{
	for (int8_t k = 0; k < NUMBER_OF_BANKS; k++)
	{
		uint8_t b = p5bytes[k];

		for (int8_t i = 0; i < 8; i++)
		{
			uint8_t ledNumber = k * 8 + i;
			uint8_t enable = b & 0x01;
			// Start timer to turn down currant later
			if (enable) {
				ToyTimer[ledNumber] = 0;
				if (g_LED[ledNumber].enable == 0) {
					DbgOut(DBGINFO, "%i ON", ledNumber); \
				}
			} else {
				if (g_LED[ledNumber].enable > 0) {
					DbgOut(DBGINFO, "%i off", ledNumber); \
				}
			}
			g_LED[ledNumber].enable = enable;			
			uint16_t value = enable ? PWM_POWER_MAX : 0;
			#ifdef KEY_MUTE_TOYS
			if (muteToys > 0) value = 0;
			#endif
			switch (g_LED[ledNumber].pwm_comparator) {
				case 'A':
					OCR4A = value;
				case 'B':
					OCR4B = value;
				case 'C':
					OCR4C = value;
				default:
					break;
			}
			b >>= 1;
		}
	}

	uint8_t pulse_speed = p5bytes[4];

	if (pulse_speed > 7)
	    pulse_speed = 7;

	if (pulse_speed == 0)
	    pulse_speed = 1;

	g_dt = pulse_speed * 128;
}


static void update_profile(int8_t k, uint8_t * p8bytes)
{
	if (k >= NUMBER_OF_BANKS)
		return;

	for (int8_t i = 0; i < 8; i++)
	{
		g_LED[k * 8 + i].mode = p8bytes[i];
	}
}


static void update_pwm(uint8_t *pwm, int8_t n, uint16_t t)
{
	for (int8_t i = 0; i < n; i++) 
	{
		if (g_LED[i].enable == 0)
		{
			pwm[i] = 0;
		}
		else
		{
			uint8_t b = g_LED[i].mode;

			if ((b >= 0) && (b <= MAX_PWM))
			{
				// constant brightness

				pwm[i] = b;
			}
			else if (b == 129)
			{
				// triangle

				uint16_t x = t >> 8;
				if (x & 0x80) // 128..255
					x = 255 - x;

				pwm[i] = (MAX_PWM * x) >> 7;

			}
			else if (b == 130)
			{
				// rect
				
				pwm[i] = (t & 0x8000) ? MAX_PWM : 0;
			}
			else if (b == 131)
			{
				// fall

				uint16_t x = 255 - (t >> 8);

				pwm[i] = (MAX_PWM * x) >> 8;
			}
			else if (b == 132)
			{
				// rise

				uint16_t x = t >> 8;

				pwm[i] = (MAX_PWM * x) >> 8;
			}
			else
			{
				// unexpected!

				pwm[i] = 0;
			}
		}
	}
}


ISR(LED_TIMER_vect)
{
	static int8_t counter = 0;
	static int8_t counterMillis = 0;
	static uint16_t t = 0;
	static uint8_t pwm[NUMBER_OF_LEDS];

	counterMillis++;
	if (counterMillis == 5) {
		counterMillis = 0;
		millis_timer++;
		int ledCheck = millis_timer % millis_ledTimersize;
		if (ledCheck == 0) {
			PORTB |= 0b10000000;
		} else if (ledCheck == millis_ledTimerOffAt) {
			PORTB &= 0b01111111;
		}
	}

	

	counter--;

	if (counter < 0)
	{
		// reset counter
		counter = MAX_PWM - 1; // pwm value of MAX_PWM should be allways 'on', 0 should be allways 'off'

		// increment time counter
		t += g_dt;

		// Xavier Hosxe
		// Turn down currant when timer reached
		// We check every 10ms
		#ifdef KEY_MUTE_TOYS
		if (muteToys == 0) 
		#endif
		{
			#define MAP(X, pin, inv, pwm_delay, COMP) \
			if (pwm_delay > 0 && g_LED[X##pin##_index].enable) { \
				if (ToyTimer[X##pin##_index] < X##pin##_pwm_delay) { \
					ToyTimer[X##pin##_index]++; \
				} else if (ToyTimer[X##pin##_index] == X##pin##_pwm_delay) { \
					OCR4##COMP = PWM_POWER_SAVING;  \
					ToyTimer[X##pin##_index]++; \
					DbgOut(DBGINFO, "%i down to %i", X##pin##_index); \
				} \
			}
			LED_MAPPING_TABLE(MAP)
			#undef MAP
		}

		// update pwm values
		update_pwm(pwm, sizeof(pwm) / sizeof(pwm[0]), t);

	}

	// set or clear all defined pins
	// if pwm_delay, it's real PWM
	#define MAP(X, pin, inv, pwm_delay, pwm_compare_register) \
	if (pwm_delay == 0) { if ((pwm[X##pin##_index] > counter) == (!inv)) { PORT##X |= (1 << pin); } else { PORT##X &= ~(1 << pin); } } 
	LED_MAPPING_TABLE(MAP)

	#undef MAP

}

//		

static void led_ports_init(void)
{
	DDRB |= 0b10000000;

	#define MAP(X, pin, inv, pwm_delay, C) \
		DDR##X  |= (1 << pin); \
		if (pwm_delay == 0) {  \
			PORT##X &= ~(1 << pin);  \
		} else {  \
			OCR4##C = 0;  \
			timer4Used = 1;  \
		}
	LED_MAPPING_TABLE(MAP)
	#undef MAP

	if (timer4Used) {
		// https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/ATmega640-1280-1281-2560-2561-Datasheet-DS40002211A.pdf
		// Spec 17-2 page 145 for WGM
		// COM4*1 : prescaler 8 
		TCCR4A = _BV(COM4A1) | _BV(COM4B1) | _BV(COM4C1) | _BV(WGM42) | _BV(WGM41); 
		TCCR4B =  _BV(CS40);
	}
}

#endif
