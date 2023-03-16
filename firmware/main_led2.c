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
#include <avr/delay.h>


int extValue = 0;

int main(void)
{
//	clock_init();

	// Led 36
	// _map_( C, 1, 0 ) /* ( A9 )                Digital pin 36 */ 
	DDRC = 0b00000010;
	DDRB = 0b10000000;

	PORTC = 0b00000010;

	while (1) {
		_delay_ms(500);
  	    PORTB = 0b00000000;
		PORTC = 0b00000010;
		
		_delay_ms(500);
    	PORTB = 0b10000000;
		PORTC = 0b00000000;		
	}
}

