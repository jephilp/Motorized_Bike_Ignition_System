/*
TTDJan23DriftTestLowSpeed.ino
5 3.7 19 22 22 20 17 14 11 8 7 ... 


TTDJan18DriftTest1.ino
This version has no ignition timing drift at all!

*/

#include <Wire.h>
//--------------------------------------------
//CTC Libraries, which are NOT used, are they?
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>
#include <U8x8lib.h>
U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // Adafruit Feather ESP8266/32u4 Boards + FeatherWing OLED
//#define DISPLAY;
//---------------------------------------------------------------------------
    volatile unsigned long _hits;
    volatile unsigned long _oneDegreeStartTime =0; // used to calculate oneDegree
    volatile unsigned long currentMicros;
    volatile unsigned long previousStartTime;
    int rpm =0;    
    int _rpm;
    int _previousRPM;
    unsigned long _startTime;
    int _rpmChange;
    int _pin;
    int _slotsPerRevolution;
    volatile unsigned long _elaspsedTime;
    volatile  long _oneDegree;   
    const int timer1Pin = 7;
    const int triggerPin = 8;
    const int strobeLed = 13;
    int coilPulsePot = A1;
    int triggerTime;
    int triggerMsec;
    volatile int triggerNorth = 0;
    volatile unsigned long delta;
    volatile int rpmTemp;
    volatile unsigned long  advance = 0; 
    long int advanceFormula;
    volatile unsigned int advanceRange; //Time range in microseconds for 1/2 of a rev.
    volatile float oneDegree;    //The time it takes for the crank to advance one degree at the current rpm, I think.
    volatile  long advancePeriod;//One element of the time to add to or subtract from 1/2 rev time before making a spark
    volatile unsigned long advanceDelay = 0;
    volatile unsigned long sparkDelay = 0;
    volatile boolean sparkOn = false; //Used to shut off sparks when not wanted.
    volatile boolean engineStopped = true;  //To be used wherever I need it.  Do these boolean variables need to be "volatile"?
    volatile boolean engineRunning =false;  //Opposite of engineStopped; To be used wherever I need it
    volatile int sparkTimingDegrees = 0;

//---------------------------------------------------------------------------


//----------------------------------------------------------------------------
void setup()
{
  #ifdef DISPLAY
    u8x8.begin();
    u8x8.setPowerSave(0);
    //With this font the only usable lines are 0-2, anything below is cut
    u8x8.setFlipMode(1);
    u8x8.clearDisplay();
    u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
    u8x8.drawString(0,1,"RPM: ");
  #endif
    
  pinMode(triggerPin, OUTPUT);
  pinMode(timer1Pin, OUTPUT);
  pinMode(strobeLed, OUTPUT);
  pinMode(coilPulsePot, INPUT); 
  
  Serial.begin(115200);
//--------------------------- 

//--------------------------  
  
 //attachInterrupt(0, beam_interrupt, FALLING);
    
 attachInterrupt(3, beam_interruptN, FALLING); 

//--------------------Timer Compare Match Code---------------------------
// initialize Timer1
cli();          // disable global interrupts while doing this
TCCR1A = 0;     // set entire TCCR1A register to 0
TCCR1B = 0;     // same for TCCR1B

// set compare match register to desired timer count:
OCR1A = advance; //Initially setting OCR1A to zero because advance is declared as zero

TCCR1B |= (1 << WGM12); // turn on CTC mode
TCCR1B = _BV(CS00) | _BV(CS01); //Prescale = 64, but a little faster, maybe

TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt:
sei(); // enable global interrupts:
}
//-----------------------------------------------------------------------
//Interrupt 0 ISR
//Capture the  3 South Hall Semsor Falling Pulse for use in reading RPMs
//Not used at present
//----------------------------------------------------------------------
void beam_interrupt()      //Capture 3 South magnets, Hall Effect sensor Falling pulse,

{
    ++_hits;
    
}

//Interrupt 1 ISR
//Capture the North Hall Sensor signal for use in trigger signal and also
//use it in reading RPMs
//------------------------------------------------------------------------
void beam_interruptN()  
{ 
  //Develop "oneDegree" by counting the time between N Magnets on the rotor
  //This is THE KEY value I use to calculate ignition timing and rpm
  currentMicros = micros();
  unsigned long elapsedTime = (currentMicros- previousStartTime);
  unsigned long etCorrected =elapsedTime +1980;
  oneDegree = etCorrected/360.0;
  previousStartTime = currentMicros;
  
  //Start timer1, using Prescale value of 64
  TCCR1B = _BV(CS00) | _BV(CS01); 
}  
 
//--------------------------------------------------
//Timer1 Compare Match ISR: This routine fires at the end of the icnition timing delay period

ISR(TIMER1_COMPA_vect)
{
  //global disable interrupts
  cli();
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12)); // Stop the counter 
  TCNT1 = 0; //Rest the counter to zero
  
  digitalWrite(7, !digitalRead(7)); //This line is here for debugging, only to toggle an LED on Pin 7
                                   //whenever timer1 reaches compare match to prove that the timer IS working
  makeSparks(); //Coil charging function.  
  sei();   //Global enable interrupts 
}

//-------------------------------------------------------------------------------
//Here's where the sparks are made; Coil Charging begins for a set time,
//times out, and ends with a spark and a Strobe LED
void makeSparks()
{
    if(sparkOn)
  {
   digitalWrite(triggerPin, HIGH); //This is where coil charging begins
   delayMicroseconds(3000); //This fixes Coil Charge peroid at 3 milliseconds (strobe 50 usec + 2950 usec)
   digitalWrite(triggerPin, LOW);  //This is where the spark actually occurs.
   
   digitalWrite(strobeLed, HIGH);
   delayMicroseconds(50); //This is the on-period for the strobe.  
   digitalWrite(strobeLed, LOW);   
  }
  
}

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


 displayRPM_Serial(); //Much shorter than I2C LCD.
 displayRPM_Display();
//Ignition delay calculations here:
//---------------------------------

if (rpm <130)
   {
    //0 to 100 rpms; too slow, no sparks
    sparkOn = false;
    sparkTimingDegrees = 0;
   }
else
if (rpm < 750)
   {
     // 101 to 750 rpms; cranking, make .5 degree advanced sparks
    advancePeriod = (oneDegree * -.5);
    sparkOn = true;
    sparkTimingDegrees = -.5;
   }
else
if (rpm < 1001)
   {
    // 750 to 1001 rpms; running okay, make between 4 and 7
    advancePeriod = (oneDegree *-7);
    sparkOn = true;
    sparkTimingDegrees = -7;
   }
else
if (rpm < 1500)
   {
    // 1001 to 1500 rpms; running okay, 15 point added because "smoothing"
    advancePeriod = (oneDegree *-15);
    sparkOn = true;
    sparkTimingDegrees = -15;
   }
else
if (rpm < 2000)
   {
    // 1501 to 2000 rpms; running okay, make 20 degree advanced sparks
    advancePeriod = (oneDegree *-20);
    sparkOn = true;
    sparkTimingDegrees = -20;
   }
if (rpm < 3000)
   {
    // 2001 to 3000 rpms; running okay, make 22 to 22.5 degree advanced sparks
    advancePeriod = (oneDegree *-22);
    sparkOn = true;
    sparkTimingDegrees = -22;
   }
else
if (rpm < 4000)
   {
    // 3001 to 4000 rpms; running okay, make 21-23 degree advanced sparks
    advancePeriod = (oneDegree *-21);
    sparkOn = true;
    sparkTimingDegrees = -21;
   }
else
if (rpm < 5000)
   {
    // 4001 to 5000 rpms; running okay, make 19-21.5 degree advanced sparks
    advancePeriod = (oneDegree *-20);
    sparkOn = true;
    sparkTimingDegrees = -20;
   }
else
if (rpm < 6000)
   {
    // 5001 to 6000 rpms; running okay, make 16-19 degree advanced sparks
    advancePeriod = (oneDegree *-17.5);
    sparkOn = true;
    sparkTimingDegrees = -17.5;
   }
else
if (rpm < 7000)
   {
    // 6001 to 7000 rpms; running okay, make 12-16 degree advanced sparks
    advancePeriod = (oneDegree *-14);
    sparkOn = true;
    sparkTimingDegrees = -14;
   }
else
if (rpm < 8000)
   {
    // 7001 to 8000 rpms; running okay, make 9-13 degree advanced sparks
    advancePeriod = (oneDegree *-11);
    sparkOn = true;
    sparkTimingDegrees = -11;
   }
else
if (rpm < 9000)
   {
    // 8001 to 9000 rpms; running okay, make 5.8-10 degree advanced sparks
    advancePeriod = (oneDegree *-8);
    sparkOn = true;
    sparkTimingDegrees = -8;
   }
else
if (rpm < 10000)
   {
    // 9001 to 10000 rpms; running okay, make 7 degree advanced sparks
    advancePeriod = (oneDegree *-7);
    sparkOn = true;
    sparkTimingDegrees = -7;
   }
else


if (rpm < 19000)
   {
     //retard spark to 40 degrees
    advancePeriod = (oneDegree * 40);
    sparkOn = true; 
    sparkTimingDegrees = 40;
  }
else
if (rpm < 20000)
//if (advanceRange <(5172 - 20))
   {// 5801 to infinity rpms; hard overspeed range; no sparks
    //sparkOn = false;
    sparkTimingDegrees = 60;
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

void displayRPM_Display()
{
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);
  u8x8.setCursor(8, 1);
  u8x8.print(rpm);
}

void displayRPM_Serial()
{
Serial.print(rpm);
/*Serial.print("  ");
Serial.print(delta);
Serial.print("  ");
Serial.print(oneDegree);
Serial.print("  ");
Serial.print(OCR1A);
Serial.print("  ");
Serial.print(OCR1A/rpm);
Serial.print("  ");
Serial.println(rpm/OCR1A);
Serial.print("  ");*/
Serial.println(sparkTimingDegrees);
}
