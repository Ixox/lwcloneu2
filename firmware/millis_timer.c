// Source: https://gist.github.com/adnbr/2439125#file-counting-millis-c
// Small modification for lwclone2u MPU6050 driver


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "millis_timer.h"

uint16_t millis_ledTimersize = 1600;
uint16_t millis_ledTimerOnAt = 800;

volatile unsigned long millis_timer;


void millis_setLedTimer(uint16_t size, uint16_t onAt) {
    millis_ledTimersize = size;
    millis_ledTimerOnAt = onAt;
}

void millis_init(void)
{
    // CTC mode, Clock/8
    TCCR3B |= (1 << WGM12) | (1 << CS11);
    
    // Load the high byte, then the low byte
    // into the output compare
    OCR3AH = (CTC_MATCH_OVERFLOW >> 8);
    OCR3AL = CTC_MATCH_OVERFLOW;
    sei();
    
    // Enable the compare match interrupt
    TIMSK3 |= (1 << OCIE1A);

    // Init led timer
 	// Init LED36 Out
	DDRB |= 0b10000000;
	// DDRB = 0b10000000;
	// PORTC = 0b00000010;
  	// PORTB = 0b10000000;

	// Led 36 ON
	// PORTC = 0b00000010;
	// Led 36 OFF
	// PORTC = 0b00000000;		

}

unsigned long millis(void)
{
    unsigned long millis_return;
    // ensure this cannnot be disrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        millis_return = millis_timer;
    }
    return millis_return;
}

ISR (TIMER3_COMPA_vect)
{
    millis_timer++;

    int ledCheck = millis_timer % millis_ledTimersize;
    if (ledCheck == 0) {
        PORTB |= 0b10000000;
    } else if (ledCheck == millis_ledTimerOnAt) {
	    PORTB &= 0b01111111;
    }
}
