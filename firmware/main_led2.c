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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "i2cmaster/i2cmaster.h"

// #define MPU_ADDR 0x68
#define MPU_ADDR 0xF0


void slowBlink() {	
	while (1) {
		_delay_ms(1000);
  	    PORTB = 0b00000000;
		
		_delay_ms(1000);
    	PORTB = 0b10000000;
	}
}

void fastBlink1() {	
	while (1) {
		_delay_ms(250);
  	    PORTB = 0b00000000;
		
		_delay_ms(250);
    	PORTB = 0b10000000;
	}
}

void fastBlink2() {	
	while (1) {
		_delay_ms(20);
  	    PORTB = 0b00000000;
		
		_delay_ms(1000);
    	PORTB = 0b10000000;
	}
}

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))


int main(void)
{
	_delay_ms(50);
	DDRB = 0b10000000;
	// SDA  D20 - PD 1
	// SCL D21 - PD 0
	// Activate internal pullups
	PORTD = 0b00000011;
  
	// cbi(TWSR, TWPS0);
  	// cbi(TWSR, TWPS1);
  	cbi(TWSR, TWPS0);
  	cbi(TWSR, TWPS1);
	i2c_init();
	_delay_ms(10);

    uint8_t mpuProblem = i2c_start(MPU_ADDR);
    if (mpuProblem == 0) {
        i2c_write(0x6B);
        i2c_write(0x00);
        i2c_stop();
		slowBlink();
    } else if (mpuProblem == 1) {
		fastBlink1();
	} else {
		fastBlink2();
	}
}