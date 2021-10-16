#ifndef TEMPO_cpp
#define TEMPO_cpp

#include "tempo.h"

Tempo Timer1;              // preinstatiate

ISR(TIMER1_COMPA_vect)          // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  Timer1.isrCallback();
}

void Tempo::initialize(long microseconds)
{
  TCCR1A = 0; // clear control register A 
  TCCR1B = _BV(WGM12); // turn on CTC mode
}

void Tempo::setPeriod(long microseconds) // setta il periodo
{
  clockSelectBits = _BV(CS11); // prescale by /8 = 0.5us at 16MHz
  oldSREG = SREG;				
  cli();							// Disable interrupts for 16 bit register access
  unsigned long cycles = ((F_CPU / 1000000) * microseconds) >> 3;
  if (cycles >= RESOLUTION) cycles = RESOLUTION - 1;
  OCR1A = cycles;
  SREG = oldSREG;
}

void Tempo::attachInterrupt(void(*isr)(), long microseconds)
{
  isrCallback = isr; // register the user's callback with the real ISR
  TIMSK1 = _BV(OCIE1A); // sets the timer overflow interrupt enable bit on compare
  //might be running with interrupts disabled (eg inside an ISR), so don't touch the global state
  //sei();
}

void Tempo::detachInterrupt()
{
  TIMSK1 &= ~_BV(OCIE1A); // clears the timer overflow interrupt enable bit, timer continues to count without calling the isr 
}

void Tempo::start()
{ 
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
  TCCR1B |= clockSelectBits; // fa partire il timer
}

void Tempo::stop()
{
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));          // clears all clock selects bits
}

unsigned long Tempo::read()		//returns the value of the timer in microseconds
{									//rember! phase and freq correct mode counts up to then down again
  	unsigned long tmp;				// AR amended to hold more than 65536 (could be nearly double this)
  	unsigned int tcnt1;				// AR added

	oldSREG= SREG;
  	cli();							
  	tmp=TCNT1;    					
	SREG = oldSREG;

	char scale=0;
	switch (clockSelectBits)
	{
	case 1:// no prescalse
		scale=0;
		break;
	case 2:// x8 prescale
		scale=3;
		break;
	case 3:// x64
		scale=6;
		break;
	case 4:// x256
		scale=8;
		break;
	case 5:// x1024
		scale=10;
		break;
	}
	
	do {	// Nothing -- max delay here is ~1023 cycles.  AR modified
		oldSREG = SREG;
		cli();
		tcnt1 = TCNT1;
		SREG = oldSREG;
	} while (tcnt1==tmp); //if the timer has not ticked yet

	//if we are counting down add the top value to how far we have counted down
	tmp = (  (tcnt1>tmp) ? (tmp) : (long)(ICR1-tcnt1)+(long)ICR1  );		// AR amended to add casts and reuse previous TCNT1
	return ((tmp*1000L)/(F_CPU /1000L))<<scale;
}

#endif