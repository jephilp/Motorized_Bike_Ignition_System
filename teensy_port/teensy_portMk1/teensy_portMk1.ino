//teensy_portMk1
//Fully functional, tested up to 20,000rpm on the bench
//Accuracy at 48Mhz clock, fastest with LTO, is +-40us
//Will have minor changes but nothing will be changed functionally
//Continued in Mk2
//Teensy LC code from tested_running
//Written by Jack Philp on 10/28/21
//Hopefully will be functional on the 4.0 and 3.6
#include <Arduino.h>
//These run the interrupts and create the loop time
    IntervalTimer sparkTimer;
    IntervalTimer heartbeatTimer;
    //These replace the subtraction and just use time counters
    elapsedMicros sinceInterrupt;
    elapsedMicros loopDelta;
    elapsedMicros calculationDelta;
//This is the old loop time, it is only for arduino
    //volatile unsigned long currentMicros;
    //volatile unsigned long previousStartTime;
//In use pins
    const int inputTrigger = 7;
    const int outputPin = 8;
    //const int strobeLed = 13;
//These are the running volatile variables since an interrupt can corrupt them
    volatile int rpm =0;  
    volatile double delta;
    volatile double  advance = 0; 
    volatile  double advancePeriod;//One element of the time to add to or subtract from 1/2 rev time before making a spark
    volatile boolean sparkOn = false; //Used to shut off sparks when not wanted.
    volatile double sparkTimingDegrees = 0;
//These variables run within interrupts so they are NOT volatile TESTING CURRENTLY FOR ISSUES
    volatile double oneDegree;    //The time it takes for the crank to advance one degree at the current rpm.
    double calcDelta;

void setup() {
  heartbeatTimer.begin(heartbeat,100000);
  heartbeatTimer.priority(1);
  pinMode(outputPin, OUTPUT);
  pinMode(inputTrigger, INPUT);
  //pinMode(strobeLed, OUTPUT);

  Serial.begin(115200);
  //Interrupt intialization created on pin inputTrigger, triggering beam_interrupt
  //While the wave is rising, this should be modified to follow the wave properly.
  attachInterrupt(digitalPinToInterrupt(inputTrigger), beam_interrupt, RISING); 
  //Start with advance at zero
  advance = 0;
  sinceInterrupt = 0;
}

//Interrupt 1 ISR
//Capture the North Hall Sensor signal for use in trigger signal and also
//use it in reading RPMs
//------------------------------------------------------------------------
void beam_interrupt()  
{ 
  calculationDelta = 0;
  //Develop "oneDegree" by counting the time between N Magnets on the rotor
  //This is THE KEY value I use to calculate ignition timing and rpm
  oneDegree = (float(sinceInterrupt)+2030)/360;
  //Serial.println(sinceInterrupt);
  sinceInterrupt = 0;
  //Start timer
  //This will have the timer wait until the time
  //put into advance period is reached or after it.
  sparkTimer.begin(makeSparks, advance); 
  //Priority is set to zero so that heartbeat can't interrupt it.
  sparkTimer.priority(0);
}  

//--------------------------------------------------
//Timer1 Compare Match ISR: This routine fires at the end of the icnition timing delay period
void makeSparks()
{
  cli();
  //Stops the timer and sets it to zero
  sparkTimer.end();
    if(sparkOn)
  {
   digitalWriteFast(outputPin, HIGH); //This is where coil charging begins
   delayMicroseconds(3000); //This fixes Coil Charge peroid at 3 milliseconds (strobe 50 usec + 2950 usec)
   digitalWriteFast(outputPin, LOW);  //This is where the spark actually occurs.
   
   //digitalWriteFast(strobeLed, HIGH);
   //delayMicroseconds(50); //This is the on-period for the strobe.  
   //digitalWriteFast(strobeLed, LOW);   
  }
  calcDelta = calculationDelta;
  sei();   //Global enable interrupts 
}
void heartbeat(){
  Serial.println("Heartbeat");
  Serial.print("RPM:");
  Serial.println(rpm);
  Serial.print("oneDegree: ");
  Serial.println(oneDegree);
  Serial.print("Timing in Degrees:");
  Serial.println(sparkTimingDegrees);
  Serial.print("Loop Delta: ");
  Serial.println(delta);
  Serial.print("Calc Delta: ");
  Serial.println(calcDelta);
  Serial.print("Timing Delay: ");
  Serial.println(advance);
  Serial.println();
}
//--------------Main Loop To Calculate RPM, Update LCD Display and Serial Monitor----------------
void loop()
{
//Beginning of Code to measure loop time
  loopDelta = 0; 
//End of Code to measure loop time 
 rpm =1000000 * 60/(oneDegree * 360); // time for one revolution;
 advance = ((((180  + sparkTimingDegrees)* oneDegree)-3000+delta)/4); //This divide by 4 works well,

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
else
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
    
   delta = loopDelta;
 //End of Code to measure loop time    
}
//------------------------------------------------------------
//End of the Loop-----End of the Loop----End Of The Loop====
//-----------------------------------------------------------
