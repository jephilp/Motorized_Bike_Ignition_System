#ifndef TEMPO_h
#define TEMPO_h

#include <avr/io.h>
#include <avr/interrupt.h>

#define RESOLUTION 65536    // Timer1 is 16 bit

class Tempo
{
  public:
  
    // properties
    unsigned char clockSelectBits;
	char oldSREG;					// To hold Status Register while ints disabled

    // methods
    void initialize(long microseconds=1000000);
    void start();
    void stop();
	unsigned long read();
    void attachInterrupt(void (*isr)(), long microseconds=-1);
    void detachInterrupt();
    void setPeriod(long microseconds);
    void (*isrCallback)();
};

extern Tempo Timer1;
#endif