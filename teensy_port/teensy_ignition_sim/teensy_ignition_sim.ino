//Teensy Ignition Simulator
//Created by: Jack Philp
//Date: 11/15/21
//Created to use a Teensy 4.0 to create
//a 100 Hz square wave to simulate a hall sensor.
//Will later have an input to read and correct.
//outPin goes to the input pin on the ignition
const int outPin = 7;
//inPin comes from the output pin on the ignition
const int inPin = 8;
//This pin connects to pin 7 between a jumper
const int waveInterrupter = 9;
//Set the frequency HERE
const int setFreq = 150;

elapsedMicros waveDelta;
elapsedMicros loopDelta;
float storedWaveDelta;
float storedLoopDelta;

IntervalTimer heartbeatTimer;

void setup() {
  pinMode(outPin, OUTPUT);
  pinMode(inPin,INPUT);
  Serial.begin(9600);
  heartbeatTimer.begin(heartbeat,100000);
  attachInterrupt(digitalPinToInterrupt(inPin), timerEnd, RISING);
  attachInterrupt(digitalPinToInterrupt(9), waveInterrupt, RISING);
  tone(outPin,setFreq);
}

void loop() {
}

void heartbeat(){
  Serial.print("Wave Distance: ");
  Serial.println(storedWaveDelta);
  Serial.print("Loop Delta: ");
  Serial.println(storedLoopDelta);
}

void timerEnd(){
  //Serial.println("Wave End");
  storedWaveDelta = waveDelta;
  //waveDelta = 0;
}

void waveInterrupt(){
  //Serial.println("Wave Start");
  waveDelta = 0;
}
