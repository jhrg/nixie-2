
/**
 * @brief Code for the PID controller for the HV PS
 */

#include <Arduino.h>
#include <PID_v1.h>

#include "hv_ps.h"

#define HV_PS_INPUT A7
#define SET_POINT 455     // ~ 200v

double input = 80, output = 50, setpoint = SET_POINT;
double kp = 0.8, ki = 0.4, kd = 0.0;

PID myPID(&input, &output, &setpoint, kp, ki, kd, DIRECT);

/**
 * @brief Setup the HV power supply
 * This sets up Timer 2 so that the PWN frequency on PIN 3 is 
 * 32k. The sample frequency is 10ms.
*/
void hv_ps_setup() {
    pinMode(HV_PS_INPUT, INPUT);

    // Use Timer2 for the HV PS control signal
    // COM2B 1:0 --> 1, 0 (non-inverted)
    // WGM22:0 --> 1 (0, 0, 1) (phase-correct PWM, 0xFF top)
    TCCR2A = 0;
    TCCR2A = _BV(COM2B1) | _BV(WGM20);
    // Set the pre-scaler at 1 (62.5 kHz)
    TCCR2B = _BV(CS20);

    // Start out with low voltage
    // OCR2B is Arduino Pin 3
    OCR2B = 0x010;  // 0-bit resolution --> 0x00 - 0xFF

    input = analogRead(HV_PS_INPUT);
    myPID.SetOutputLimits(10, 150);
    myPID.SetSampleTime(SAMPLE_PERIOD);
    myPID.SetMode(AUTOMATIC);  // This turns on the PID; MANUAL mode turns it off
}

/**
 * @brief compute on iteration of the PID controller.
 * The sample period is 10ms. The PID controller will only compute
 * at that rate, but we can save time by only calling the ADC when the
 * PID controller needs a new value.
 */
void hv_ps_adjust() {
    input = analogRead(HV_PS_INPUT);
#if PID_DIAGNOSTIC
    PORTD |= _BV(PORTD6);
#endif
    myPID.Compute();
#if PID_DIAGNOSTIC
    PORTD &= ~_BV(PORTD6);
#endif
    OCR2B = (unsigned char)output;
}
