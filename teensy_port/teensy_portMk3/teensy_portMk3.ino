//teensy_portMk3
//Created from Mk2 to become more effecient
//Also fix compatibility on the 4.0
//Written by Jack Philp on 11/8/21
//Written for use on a teensy LC and 4.0
#include <Arduino.h>
//Timing Array
//New feature for ease of use and code shrinkage
const float timingDeg[] = {0,-5,-7,-16.5,-19,-20.2,-20.5,-20.2,-19.5,-18,-16.8,-15.2,-14};
 const int rpmArray[] = {100,750,1000,1500,2000,3000,4000,5000,6000,7000,8000,9000,10000};
//            Index array[0 , 1 , 2  ,  3 ,  4 ,  5 ,  6 , 7  , 8  ,  9 , 10,  11 , 12  ]
//These run the interrupts and create the loop time
    IntervalTimer sparkTimer;
    IntervalTimer heartbeatTimer;
    //These replace the subtraction and just use time counters
    elapsedMicros sinceInterrupt;
    elapsedMicros loopDelta;
    elapsedMicros calculationDelta;
    elapsedMicros dwell;
//In use pins
    const int inputTrigger = 7;
    const int outputPin = 8;
//These are the running volatile variables since an interrupt can corrupt them
    volatile int rpm =0;  
    volatile float delta;
    volatile float  advance = 0; 
    volatile boolean sparkOn = false; //Used to shut off sparks when not wanted.
    volatile float sparkTimingDegrees = 0;
    volatile float oneDegree;    //The time it takes for the crank to advance one degree at the current rpm.
//These are new variables for mk2
    volatile float lowRpm;
    volatile float highRpm;
    volatile float lowDeg;
    volatile float highDeg;
    volatile float calcDelta;

void setup() {
  heartbeatTimer.begin(heartbeat,100000);
  sparkTimer.priority(0);
  heartbeatTimer.priority(1);
  pinMode(outputPin, OUTPUT);
  pinMode(inputTrigger, INPUT);
  Serial.begin(115200);
  //Interrupt intialization created on pin inputTrigger, triggering beam_interrupt
  //While the wave is rising, this should be modified to follow the wave properly.
  attachInterrupt(digitalPinToInterrupt(inputTrigger), signalInterrupt, RISING); 
  //Start with advance at zero
  advance = 0;
  sinceInterrupt = 0;
}

//Interrupt on hall sensor on crankshaft
//Receives, calculates then begins the fire timer
//------------------------------------------------------------------------
void signalInterrupt()  
{ 
  //Turns off interrupts so that the function runs without issue
  cli();
  oneDegree = (float(sinceInterrupt)+2030)/360;
  advance = ((((180  + sparkTimingDegrees)* oneDegree)-(3000+(delta+calcDelta)))); //Calculates the advance
  //Starts secondary spark timer
  sinceInterrupt = 0;
  sparkTimer.begin(sparkFire,advance);
  sei();   //Reenables interrupts 
}  
void sparkFire()
{
  cli();
  //Stops the timer and sets it to zero
  sparkTimer.end();
    if(sparkOn)
  {
   digitalWriteFast(outputPin, HIGH); //This is where coil charging begins
   delayMicroseconds(3000); //This fixes Coil Charge peroid at 3 milliseconds
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
  Serial.println(-sparkTimingDegrees);
  Serial.print("Loop Delta: ");
  Serial.println(delta);
  Serial.print("Calc Delta: ");
  Serial.println(calcDelta);
  Serial.print("Timing Delay: ");
  Serial.println(advance);
  Serial.println();
  
  //Serial.println(-sparkTimingDegrees);
}
//--------------Main Loop To Calculate RPM, Update LCD Display and Serial Monitor----------------
void loop()
{
//Beginning of Code to measure loop time
  loopDelta = 0; 
//---------------------------------------------------
  calculationDelta = 0;
//Serial.println("Interrupt Fire");
//Develop "oneDegree"
  rpm = 1000000 * 60/(oneDegree * 360); // time for one revolution;
  //This is a large float calculation so if you have a low power mcu change it to longs
  sparkTimingDegrees = (rpm - lowRpm) * (highDeg - lowDeg) / (highRpm -  lowRpm) + lowDeg;//Pulls these values from the the main loop
//Checks how long it took to calculate this interrupt
  calcDelta = calculationDelta;
//Follows the curve plotted from Jaguar CDI
//Ignition delay calculations here:
//---------------------------------
//Negative values are advance
//Positive values are retard
if (rpm <rpmArray[0])
   {
    //0 to 100 rpms; too slow, no sparks
    sparkOn = false;
    sparkTimingDegrees = 0;
   }
else
if (rpm < rpmArray[1])
   {
     // 101 to 750 rpms;
    sparkOn = true;
    lowRpm = rpmArray[0]+1;
    highRpm = rpmArray[1];
    lowDeg = timingDeg[0];
    highDeg = timingDeg[1];
   }
else
if (rpm < rpmArray[2])
   {
    // 751 to 1000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[1]+1;
    highRpm = rpmArray[2];
    lowDeg = timingDeg[1];
    highDeg = timingDeg[2];
   }
else
if (rpm < rpmArray[3])
   {
    // 1001 to 1500 rpms;
    sparkOn = true;
    lowRpm = rpmArray[2]+1;
    highRpm = rpmArray[3];
    lowDeg = timingDeg[2];
    highDeg = timingDeg[3];
   }
else
if (rpm < rpmArray[4])
   {
    // 1501 to 2000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[3]+1;
    highRpm = rpmArray[4];
    lowDeg = timingDeg[3];
    highDeg = timingDeg[4];
   }
else
if (rpm < rpmArray[5])
   {
    // 2001 to 3000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[4]+1;
    highRpm = rpmArray[5];
    lowDeg = timingDeg[4];
    highDeg = timingDeg[5];
   }
else
if (rpm < rpmArray[6])
   {
    // 3001 to 4000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[5]+1;
    highRpm = rpmArray[6];
    lowDeg = timingDeg[5];
    highDeg = timingDeg[6];
   }
else
if (rpm < rpmArray[7])
   {
    // 4001 to 5000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[6]+1;
    highRpm = rpmArray[7];
    lowDeg = timingDeg[6];
    highDeg = timingDeg[7];
   }
else
if (rpm < rpmArray[8])
   {
    // 5001 to 6000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[7]+1;
    highRpm = rpmArray[8];
    lowDeg = timingDeg[7];
    highDeg = timingDeg[8];
   }
else
if (rpm < rpmArray[9])
   {
    // 6001 to 7000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[8]+1;
    highRpm = rpmArray[9];
    lowDeg = timingDeg[8];
    highDeg = timingDeg[9];
   }
else
if (rpm < rpmArray[10])
   {
    // 7001 to 8000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[9]+1;
    highRpm = rpmArray[10];
    lowDeg = timingDeg[9];
    highDeg = timingDeg[10];
   }
else
if (rpm < rpmArray[11])
   {
    // 8001 to 9000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[10]+1;
    highRpm = rpmArray[11];
    lowDeg = rpmArray[10];
    highDeg = rpmArray[11];
   }
else
if (rpm < rpmArray[12])
   {
    // 9001 to 10000 rpms;
    sparkOn = true;
    lowRpm = rpmArray[11]+1;
    highRpm = rpmArray[12];
    lowDeg = rpmArray[11];
    highDeg = rpmArray[12];
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
