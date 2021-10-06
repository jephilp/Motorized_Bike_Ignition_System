/*
TTDJan23DriftTestLowSpeed.ino
This is some more clean up of TTDJan22DriftTestLowSpeed.
I am preparing to figure out how to calculate oneDegree with every S magnet
Here I change the ignition timing range to test
the retarding code within the range of 3300 rpm, 
which is all I can get out of my simulator right now.
No sparks below 130 rpm
10 degrees  retard from 131 rpm to  1000 rpm
20 degrees advance from 1001 rpm to 2600 rpm
40 degrees retard (Overspeed soft limit) from 2601 to 2900 rpm,
60 degrees retard (Overspeed hard limit) from 2901 to max rpm

TTDJan18DriftTest1.ino
This version has no ignition timing drift at all!

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//--------------------------------------------
//CTC Libraries, which are NOT used, are they?
#include <avr/io.h>
#include <avr/interrupt.h>

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
LiquidCrystal_I2C lcd(0x27, 2, 1, 0,4 ,5, 6, 7, 3, POSITIVE);  // set the LCD I2C address

//----------------------------------------------------------------------------
void setup()
{
  pinMode(triggerPin, OUTPUT);
  pinMode(timer1Pin, OUTPUT);
  pinMode(strobeLed, OUTPUT);
  pinMode(coilPulsePot, INPUT); 
  
  Serial.begin(115200);
//--------------------------- 
  lcd.begin(20,4);
  lcd.backlight();
  lcd.clear();
  lcd.begin(20, 4);     // set up the LCD's number of columns and rows: 
  lcd.print("RPM: 1Degr TCNT1 N");
  lcd.setCursor(0, 2);
  lcd.print("Adv.Range");
  lcd.setCursor (7, 2);
  lcd.print("Loop");
//--------------------------  
  
 attachInterrupt(0, beam_interrupt, FALLING);
    
 pinMode(3, INPUT);
 attachInterrupt(1, beam_interruptN, FALLING); 

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

// displayRPM_LCD(); //Only turned on for testing.  Takes too long!!!
 displayRPM_Serial(); //Much shorter than I2C LCD.

//Ignition delay calculations here:
//---------------------------------

if (rpm <130)
   {
    //0 to 100 rpms; too slow, no sparks
    sparkOn = false;
    sparkTimingDegrees = 10;
   }
else
if (rpm < 1001)
   {
     // 101 to 1000 rpms; cranking, make 10 degree retarded sparks
    advancePeriod = (oneDegree * 10);
    sparkOn = true;
    sparkTimingDegrees = 10;
   }
else
if (rpm < 2600)
   {
    // 1001 to 5600 rpms; running okay, make 20 degree advanced sparks
    advancePeriod = (oneDegree *-20);
    sparkOn = true;
    sparkTimingDegrees = -20;
   }
else
if (rpm < 2900)
   {
     //retard spark to 40 degrees
    advancePeriod = (oneDegree * 40);
    sparkOn = true; 
    sparkTimingDegrees = 40;
  }
else
if (rpm < 3000)
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

void displayRPM_LCD()  //IF it is turned on!!!
{
  lcd.setCursor(0, 1);
  lcd.print("      ");  //To clear the previous reading
  lcd.setCursor(0, 1);
  lcd.print(rpm);
  lcd.setCursor(0, 3);
  lcd.print("     ");
  lcd.setCursor(0, 3); 
   lcd.print(delta); 
 }

void displayRPM_Serial()
{
Serial.print(rpm);
Serial.print("  ");
Serial.print(delta);
Serial.print("  ");
Serial.print(oneDegree);
Serial.print("  ");
Serial.print(OCR1A);
Serial.print("  ");
Serial.print(OCR1A/rpm);
Serial.print("  ");
Serial.println(rpm/OCR1A);
}
