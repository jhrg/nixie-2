
/**
 * @brief Code for the PID controller for the HV PS
 *
 * Use a PID controller to vary the duty cycle of a 31.25 kHz 5v output
 * on Pin 10 of an ATmega348 Arduino so that it controls the High Voltage
 * power supply for the nixies tubes. The PID controller reads the HV PS
 * voltage via analog pin 7 (via a voltage divider). It uses a 10ms sample
 * period.
 */

#include "hv_ps.h"

#include <Arduino.h>
#include <PID_v1.h>

#define PID_DIAGNOSTIC 0  // PID timing on Pin 6 if 1. See below.
#define HV_PS_INPUT A7
#ifndef SAMPLE_PERIOD
#define SAMPLE_PERIOD 10  // ms, also defined in hv_ps.h
#endif
#define SET_POINT 455  // 0-1023 from the ADC; 455 ~ 200v
// The timer/counter uses 9-bit resolution --> 0x0000 - 0x01FF (0 - 511)
#define INITIAL_VALUE 0x8F  // 143, base 10, ~28% duty cycle PWM

// PID controller initial values
double input = 80, output = 50, setpoint = SET_POINT;
// PID controller constants
double kp = 0.8, ki = 0.4, kd = 0.0;
// DIRECT: controller output will increase when error is positive
PID myPID(&input, &output, &setpoint, kp, ki, kd, DIRECT);

/**
 * @brief Setup the HV power supply
 * This sets up Timer 1, the 16-bit timer capable of PWN operation.
 * The PWN signal is output to Pin 10 of the ATmega 348 Arduino.
 *
 * @see ATmega48 documentation, p.120, for information about Timer 1
 */
void hv_ps_setup() {
    pinMode(HV_PS_INPUT, INPUT);

    cli();

    TCCR1A = 0;  // initialize TCCR1A (Timer/Counter1 Control Register A) to 0
    TCCR1B = 0;  // same for TCCR1B
    TCNT1 = 0;   // initialize counter value to 0 (Timer Counter 1)

    // Use Timer1 for the HV PS control signal. This is PWM output that
    // controls the voltage of the power supply (See P.140-3).

    // COM1A (Compare Output Mode for Counter 1, Channel A), control the Output
    // Compare pin OC1A.
    // COM1B Same for channel B (which is the channel this code uses).

    // COM1A 1:0 -> 0, 0 -> Normal port operation, OC1A disconnected
    // COM1B 1:0 -> 1, 0 -> Clear OC1B on Compare Match (Set output to low level).

    // WGM1 (Waveform Generation Mode, Timer/Counter 1) The four bits that control
    // waveform generation are split between TCCR1A and TCCR1B.

    // To set the timer for 9-bit resolution. WGM1 3:0 -> 0, 1, 1, 0
    // With 9-bit res. the counter will count to 0x1FF.

    // CS1 (Clock Set, timer/counter 1) CS1 -> 0, 0, 1 -> No pre scaling. This
    // sets the clock at 62.5 kHz which provides a 31.25 kHz PWM output.

    TCCR1A = _BV(COM1B1) | _BV(WGM11);
    TCCR1B = _BV(WGM12) | _BV(CS10);

    // OCR1B (Output Compare Register 1 B) is an unsigned 16 bit int.

    OCR1B = INITIAL_VALUE; // 9-bit resolution --> 0x0000 - 0x01FF

    sei();

    // Configure the PID controller.
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
 * @brief compute an iteration of the PID controller.
 *
 * Use the PID controller to adjust the duty cycle of the PWM signal
 * controlling the HV power supply.
 *
 * The sample period of the PID controller is 10ms. The PID controller
 * code will only compute at that rate, and will only call the read_input()
 * function at that interval regardless of how many times hv_ps_adjust()
 * is called. However, the millis() function is called every time the
 * controller Compute() function is called.
 *
 * The PWM of Timer/Counter 1 is controlled by setting a value in OCR1B
 * (Output Control Register 1 B)
 *
 * PID_DIAGNOSTIC = 1 will toggle pin 6 on the arduino while the PID is
 * computing.
 */
void hv_ps_adjust() {

#if PID_DIAGNOSTIC
    PORTD |= _BV(PORTD6);
#endif

    myPID.Compute(read_input);

#if PID_DIAGNOSTIC
    PORTD &= ~_BV(PORTD6);
#endif

    // Set OCR1B (i.e., adjust the output duty cycle) using the output of
    // the PID controller.
    OCR1B = (unsigned char)output;
}
