//teensy_portMk2
//Created from Mk1 to test the map function for interpolation
//Written by Jack Philp on 10/29/21
//Written for use on a teensy LC
//Hopefully will be functional on the 4.0 and 3.6
#include <Arduino.h>
//These run the interrupts and create the loop time
    IntervalTimer sparkTimer;
    IntervalTimer heartbeatTimer;
    //These replace the subtraction and just use time counters
    elapsedMicros sinceInterrupt;
    elapsedMicros loopDelta;
    elapsedMicros calculationDelta;
//In use pins
    const int inputTrigger = 8;
    const int outputPin = 7;
//These are the running volatile variables since an interrupt can corrupt them
    volatile int rpm =0;  
    volatile double delta;
    volatile double  advance = 0; 
    volatile  double advancePeriod;//One element of the time to add to or subtract from 1/2 rev time before making a spark
    volatile boolean sparkOn = false; //Used to shut off sparks when not wanted.
    volatile double sparkTimingDegrees = 0;
    volatile double oneDegree;    //The time it takes for the crank to advance one degree at the current rpm.
//These are new variables for mk2
    volatile float lowRpm;
    volatile float highRpm;
    volatile float lowDeg;
    volatile float highDeg;
//These variables run within interrupts so they are NOT volatile TESTING CURRENTLY FOR ISSUES
    double calcDelta;

void setup() {
  heartbeatTimer.begin(heartbeat,100000);
  heartbeatTimer.priority(1);
  pinMode(outputPin, OUTPUT);
  pinMode(inputTrigger, INPUT);
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
  //Turns off interrupts so that the function runs without issue
  cli();
  
  //Stops the timer and sets it to zero
  sparkTimer.end();

  //Checks to see if spark on condition is met
    if(sparkOn)
  {
   //digitalWriteFast is a teensy high speed function
   digitalWriteFast(outputPin, HIGH); //This is where coil charging begins
   delayMicroseconds(3000); //This fixes Coil Charge peroid at 3 milliseconds (strobe 50 usec + 2950 usec)
   digitalWriteFast(outputPin, LOW);  //This is where the spark actually occurs.
   
   //digitalWriteFast(strobeLed, HIGH);
   //delayMicroseconds(50); //This is the on-period for the strobe.  
   //digitalWriteFast(strobeLed, LOW);   
  }
  //Checks how long it took to calculate this interrupt
  calcDelta = calculationDelta;
  sei();   //Global enable interrupts 
}
void heartbeat(){
  /*
  Serial.println("Heartbeat");
  Serial.print("RPM:");
  Serial.println(rpm);
  Serial.print("oneDegree: ");
  Serial.println(oneDegree);
  Serial.print("Timing in Degrees:");
  Serial.println(-sparkTimingDegrees);
  Serial.print("Loop Delta: ");
  Serial.println(delta);
  Serial.print("Calc Delta: ");
  Serial.println(calcDelta);
  Serial.print("Timing Delay: ");
  Serial.println(advance-150);
  Serial.println();
  */
  Serial.println(-sparkTimingDegrees);
}
//--------------Main Loop To Calculate RPM, Update LCD Display and Serial Monitor----------------
void loop()
{
//Beginning of Code to measure loop time
  loopDelta = 0; 
  
//End of Code to measure loop time 
 rpm =1000000 * 60/(oneDegree * 360); // time for one revolution;
 sparkTimingDegrees = (rpm - lowRpm) * (highDeg - lowDeg) / (highRpm -  lowRpm) + lowDeg;
 advance = ((((180  + sparkTimingDegrees)* oneDegree)-3000+delta)/4); //This divide by 4 works well,
//---------------------------------------------------
//This has been changed from Mk1 to incorporate map()
//Completely removed in loop calculation.
//Follows the curve plotted from Jaguar CDI
//Ignition delay calculations here:
//---------------------------------
//Negative values are advance
//Positive values are retard
if (rpm <130)
   {
    //0 to 100 rpms; too slow, no sparks
    sparkOn = false;
    sparkTimingDegrees = 0;
   }
else
if (rpm < 750)
   {
     // 101 to 750 rpms;
    sparkOn = true;
    lowRpm = 101;
    highRpm = 4000;
    lowDeg = 0;
    highDeg = -5;
    //sparkTimingDegrees = -5;
   }
else
if (rpm < 1000)
   {
    // 751 to 1000 rpms;
    sparkOn = true;
    lowRpm = 751;
    highRpm = 1000;
    lowDeg = -5;
    highDeg = -7;
    //sparkTimingDegrees = -7;
   }
else
if (rpm < 1500)
   {
    // 1001 to 1500 rpms;
    sparkOn = true;
    lowRpm = 1001;
    highRpm = 1500;
    lowDeg = -7;
    highDeg = -16.5;
    //sparkTimingDegrees = -16.5;
   }
else
if (rpm < 2000)
   {
    // 1501 to 2000 rpms;
    sparkOn = true;
    lowRpm = 1501;
    highRpm = 2000;
    lowDeg = -16.5;
    highDeg = -19;
    //sparkTimingDegrees = -19;
   }
else
if (rpm < 3000)
   {
    // 2001 to 3000 rpms;
    sparkOn = true;
    lowRpm = 2001;
    highRpm = 3000;
    lowDeg = -19;
    highDeg = -20.2;
    //sparkTimingDegrees = -20.2;
   }
else
if (rpm < 4000)
   {
    // 3001 to 4000 rpms;
    sparkOn = true;
    lowRpm = 3001;
    highRpm = 4000;
    lowDeg = -20.2;
    highDeg = -20.5;
    //sparkTimingDegrees = -20.5;
   }
else
if (rpm < 5000)
   {
    // 4001 to 5000 rpms;
    sparkOn = true;
    lowRpm = 4001;
    highRpm = 5000;
    lowDeg = -20.5;
    highDeg = -20.2;
    //sparkTimingDegrees = -20.2;
   }
else
if (rpm < 6000)
   {
    // 5001 to 6000 rpms;
    sparkOn = true;
    lowRpm = 5001;
    highRpm = 6000;
    lowDeg = -20.2;
    highDeg = -19.5;
    //sparkTimingDegrees = -19.5;
   }
else
if (rpm < 7000)
   {
    // 6001 to 7000 rpms;
    sparkOn = true;
    lowRpm = 6001;
    highRpm = 7000;
    lowDeg = -20.2;
    highDeg = -18;
    //sparkTimingDegrees = -18;
   }
else
if (rpm < 8000)
   {
    // 7001 to 8000 rpms;
    sparkOn = true;
    lowRpm = 7001;
    highRpm = 8000;
    lowDeg = -18;
    highDeg = -16.8;
    //sparkTimingDegrees = -16.8;
   }
else
if (rpm < 9000)
   {
    // 8001 to 9000 rpms;
    sparkOn = true;
    lowRpm = 8001;
    highRpm = 9000;
    lowDeg = -16.8;
    highDeg = -15.2;
    //sparkTimingDegrees = -15.2;
   }
else
if (rpm < 10000)
   {
    // 9001 to 10000 rpms;
    sparkOn = true;
    lowRpm = 9001;
    highRpm = 10000;
    lowDeg = -15.2;
    highDeg = -14;
    //sparkTimingDegrees = -14;
   }
else

if (rpm < 20000)
//if (advanceRange <(5172 - 20))
   {// 5801 to infinity rpms; hard overspeed range; no sparks
    sparkOn = false;
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
