
/**
 * @brief Code for the PID controller for the HV PS
 */

#include "hv_ps.h"

#include <Arduino.h>
#include <PID_v1.h>

#define HV_PS_INPUT A7
#define SAMPLE_PERIOD 10   // ms
#define SET_POINT 455      // ~ 200v
#define INITIAL_VALUE 0x8F // 9-bit resolution --> 0x0000 - 0x01FF

double input = 80, output = 50, setpoint = SET_POINT;
double kp = 0.8, ki = 0.4, kd = 0.0;

PID myPID(&input, &output, &setpoint, kp, ki, kd, DIRECT);

/**
 * @brief Setup the HV power supply
 * This sets up Timer 2 so that the PWN frequency on PIN 3 is
 * 32k. The sample period is 10ms.
 */
void hv_ps_setup() {
    pinMode(HV_PS_INPUT, INPUT);

    cli();

    TCCR1A = 0; // set entire TCCR2A register to 0
    TCCR1B = 0; // same for TCCR0B
    TCNT1 = 0;  // initialize counter value to 0

    // Use Timer1 for the HV PS control signal
    // Set the timer to Fast PWM. COM1A1:0 --> 1, 0
    // Set the timer for 10-bit resolution. WGM13:0 --> 0, 1, 1, 1
    // Set the timer for 9-bit resolution. WGM13:0 --> 0, 1, 1, 0
    TCCR1A = _BV(COM1B1) | _BV(WGM11);
    // Set the pre-scaler at 1 (62.5 kHz) and the two high-order bits of WGM
    TCCR1B = _BV(WGM12) | _BV(CS10);

    OCR1B = INITIAL_VALUE; // 9-bit resolution --> 0x0000 - 0x01FF

    sei();

    input = analogRead(HV_PS_INPUT);
    myPID.SetOutputLimits(10, 200);
    myPID.SetSampleTime(SAMPLE_PERIOD);
    myPID.SetMode(AUTOMATIC); // This turns on the PID; MANUAL mode turns it off
}

/**
 * @brief The simplest input reader. Always reads a value
 */
bool read_input(double *in) {
    *in = analogRead(HV_PS_INPUT);
    return true;
}

/**
 * @brief compute on iteration of the PID controller.
 * The sample period is 10ms. The PID controller will only compute
 * at that rate, but we can save time by only calling the ADC when the
 * PID controller needs a new value.
 */
void hv_ps_adjust() {

#if PID_DIAGNOSTIC
    PORTD |= _BV(PORTD6);
#endif

    myPID.Compute(read_input);

#if PID_DIAGNOSTIC
    PORTD &= ~_BV(PORTD6);
#endif

    // OCR1B is Pin 10
    OCR1B = (unsigned char)output;
}
