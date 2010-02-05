#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>

#include "clock.h"

//Counted time
volatile clock_time_t clock_datetime = 0;

//Overflow interrupt
ISR(TIMER0_COMPA_vect)
{
	clock_datetime += 1;
}

//Initialise the clock
void clock_init()
{
	OCR0A  = ((F_CPU / 1024) / 100);
	TCCR0B = ((1 << WGM01) | (1 << CS02) | (1 << CS00));
	TIMSK0 = (1 << OCIE0A);
}

//Return time
clock_time_t clock_time()
{
	clock_time_t time;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		time = clock_datetime;
	}

	return time;
}
