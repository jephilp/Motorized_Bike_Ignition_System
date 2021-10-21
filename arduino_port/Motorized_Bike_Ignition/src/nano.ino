/*
This code is modified from the origina to run on an arduino nano.
It is lightened and all unused variable are removed, along with pot control.
Define statements control debugging below.
Has floating input issue but will be resolved later.
Currently compiles way faster than the original, may due to optimization.
*/

//#include <Wire.h>
//--------------------------------------------
//CTC Libraries, which are NOT used, are they?
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>
//---------------------------------------------------------------------------
    volatile unsigned long currentMicros;
    volatile unsigned long previousStartTime;
    int rpm =0;    
   //Assigned Pins
    const int timer1Pin = 7;
    const int triggerPin = 8;
    const int strobeLed = 5;
    const int interruptPin = 3;
    const int blinkPin = triggerPin;
   //
    volatile unsigned long delta;
    volatile unsigned long  advance = 0; 
    volatile unsigned int advanceRange; //Time range in microseconds for 1/2 of a rev.
    volatile float oneDegree;    //The time it takes for the crank to advance one degree at the current rpm, I think.
    volatile  long advancePeriod;//One element of the time to add to or subtract from 1/2 rev time before making a spark
    volatile unsigned long advanceDelay = 0;
    volatile unsigned long sparkDelay = 0;
    volatile boolean sparkOn = false; //Used to shut off sparks when not wanted.
    volatile boolean engineStopped = true;  //To be used wherever I need it.  Do these boolean variables need to be "volatile"?
    volatile boolean engineRunning =false;  //Opposite of engineStopped; To be used wherever I need it
    volatile int sparkTimingDegrees = 0;

//----------------------------------------------------------------------------
void setup()
{
   
    
  pinMode(triggerPin, OUTPUT);
  pinMode(timer1Pin, OUTPUT);
  pinMode(strobeLed, OUTPUT);
  pinMode(interruptPin,INPUT);
  blink(1000);
  blink(1000);
//--------------------------  
//Assigns pin 3 to an interrupt pin, DIRECT PINS ONLY WORK ON UNO
//Looks for a falling wave form or the tail end of the magnet
 attachInterrupt(digitalPinToInterrupt(interruptPin), hallSensor_interrupt, FALLING); 

//--------------------Timer Compare Match Code---------------------------
// initialize Timer1
cli();          // disable global interrupts while doing this

TCCR1A = 0;     // set entire TCCR1A register to 0
TCCR1B = 0;     // same for TCCR1B
TCCR1C = 0;     // Added for the ATTINY84
// set compare match register to desired timer count:
OCR1A = advance; //Initially setting OCR1A to zero because advance is declared as zero

TCCR1B |= (1 << WGM12); // turn on CTC mode
TCCR1B = _BV(CS00) | _BV(CS01); //Prescale = 64, but a little faster, maybe

TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt:
sei(); // enable global interrupts:
}
//-----------------------------------------------------------------------
//Interrupt 1 ISR
//Capture the North Hall Sensor signal for use in trigger signal and also
//use it in reading RPMs
//------------------------------------------------------------------------
void hallSensor_interrupt()  
{ 
  //Develop "oneDegree" by counting the time between N Magnets on the rotor
  //This is THE KEY value I use to calculate ignition timing and rpm
  currentMicros = micros();
  unsigned long elapsedTime = (currentMicros- previousStartTime);
  unsigned long etCorrected =elapsedTime +1980;
  oneDegree = etCorrected/360.0;
  previousStartTime = currentMicros;
  blink(500);
  //Start timer1, using Prescale value of 64
  TCCR1B = _BV(CS00) | _BV(CS01); 
}  
 
//--------------------------------------------------
//Timer1 Compare Match ISR: This routine fires at the end of the icnition timing delay period

ISR(TIM1_COMPA_vect)
{
  //global disable interrupts
  cli();
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12)); // Stop the counter 
  TCNT1 = 0; //Rest the counter to zero
  
  blink(3000);                                //whenever timer1 reaches compare match to prove that the timer IS working
  //makeSparks(); //Coil charging function.  
  sei();   //Global enable interrupts 
}

//-------------------------------------------------------------------------------
//Here's where the sparks are made; Coil Charging begins for a set time,
//times out, and ends with a spark and a Strobe LED
/*
void makeSparks()
{
    if(sparkOn)
  {
   digitalWrite(blinkPin, HIGH); //This is where coil charging begins
   delayMicroseconds(3000); //This fixes Coil Charge peroid at 3 milliseconds (strobe 50 usec + 2950 usec)
   digitalWrite(blinkPin, LOW);  //This is where the spark actually occurs.  
  }
  
}
*/
//--------------Main Loop To Calculate RPM, Update LCD Display and Serial Monitor----------------
void loop()
{
//Beginning of Code to measure loop time
  unsigned long startLoopCount = micros(); 
//End of Code to measure loop time 
 rpm =1000000 * 60/(oneDegree * 360); // time for one revolution;
 advance = ((((180  + sparkTimingDegrees)* oneDegree)-3000)/4); //This divide by 4 works well,
//but still gives me OCR1A overlfow at about 127 rpm, but I can live with that.
 OCR1A = advance; 

//Ignition delay calculations here:
//---------------------------------

if (rpm <600)
   {
    //0 to 100 rpms; too slow, no sparks
    sparkOn = false;
    sparkTimingDegrees = 0;
   }
else
if (rpm > 600)
   {
     // 101 to 750 rpms; cranking, make .5 degree advanced sparks
    advancePeriod = (oneDegree * -.5);
    sparkOn = true;
    sparkTimingDegrees = -.5;
   }

//End ignition delay code
//-----------------------
//Last part of Code to measure loop time
    unsigned long end = micros();
   delta = end - startLoopCount;
 //End of Code to measure loop time  
}
//------------------------------------------------------------
//End of the Loop-----End of the Loop----End Of The Loop====
//-----------------------------------------------------------
static void blink(int blinkTime){
   digitalWrite(blinkPin,HIGH);
   delay(blinkTime);
   digitalWrite(blinkPin,LOW);
}