#include <Arduino.h>
/**
 * A simple PWM/signal generator that permits setting of the frequency
 * in Herz and the pulse width (duty cycle). It uses a bit banging method
 * to generate the signal. This sketch can also be used to generate a PWM 
 * signal on any digital pin.
 * 
 * Author: Mario Gianota July 2020
 */
#define OUTPUT_PIN 2  // PWM/Signal output pin
float frequency;        // Frequency in Herz
float dutyCycle;      // Duty cycle (pulse width) percentage

void setup() {
  pinMode(OUTPUT_PIN, OUTPUT);

  // Set the frequency and the duty cycle. For most
  // purposes you will want to leave the duty cycle set
  // to 50%.
  frequency = 3;
  dutyCycle = 50;
}

void loop() {

  // Calculate the period and the amount of time the output is on for (HIGH) and 
  // off for (LOW).
  double period = 1000000 / frequency;
  double offFor = period - (period * (dutyCycle/100));
  double onFor = period - offFor;


  if( period > 16383 ) {
    // If the period is greater than 16383 then use the millisecond timer delay,
    // otherwise use the microsecond timer delay. Currently, the largest value that
    // will produce an accurate delay for the microsecond timer is 16383.

    digitalWrite(OUTPUT_PIN, HIGH);
    delay((long)onFor/1000);
    
    digitalWrite(OUTPUT_PIN, LOW);
    delay((long)offFor/1000);
  } else {
    digitalWrite(OUTPUT_PIN, HIGH);
    delayMicroseconds((long)onFor);
    
    digitalWrite(OUTPUT_PIN, LOW);
    delayMicroseconds((long)offFor);
  }
}