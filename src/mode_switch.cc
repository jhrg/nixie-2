
#include <Arduino.h>

#include "mode_switch.h"

volatile unsigned long control_mode_switch_time = 0;
#define MODE_SWITCH_INTERVAL 100 // ms
#define LONG_MODE_SWITCH_PRESS 2000

// Is the timed_mode_switch ISR triggered on the rising or falling
// edge of the interrupt?
volatile int control_mode_switch_duration = 0;

// The next two functions are ISRs that implement the mode-switch.
// Forward declaration
void timed_mode_switch_release();

/**
 * First ISR for the mode switch. Triggered when the switch is pressed.
 * The mode switch GPIO is held HIGH normally and a button press causes
 * the input to go LOW. The ISR is triggered on the falling edge of
 * the interrupt. Capture the time and set the duration to zero. Then
 * register a second ISR for the button release, which will be triggered
 * when the GPIO pin state goes back to the HIGH level.
 *
 * @note Interrupts are disabled in ISR functions and millis() does
 * not advance. This, the value of millis() will be the value just before
 * entering this ISR.
 */
void timed_mode_switch_push()
{
    if (millis() > control_mode_switch_time + MODE_SWITCH_INTERVAL)
    {
        Serial.println("timed_mode_switch_push");
        // Triggered on the rising edge is the button press; start the timer
        control_mode_switch_time = millis();
        control_mode_switch_duration = 0;
        attachInterrupt(digitalPinToInterrupt(MODE_SWITCH), timed_mode_switch_release, FALLING);
    }
}

/**
 * When the mode switch is released, this ISR is run.
 */
void timed_mode_switch_release()
{
    if (millis() > control_mode_switch_time + MODE_SWITCH_INTERVAL)
    {
        Serial.println("timed_mode_switch_release");
        attachInterrupt(digitalPinToInterrupt(MODE_SWITCH), timed_mode_switch_push, RISING);
        control_mode_switch_duration = millis() - control_mode_switch_time;
        control_mode_switch_time = millis();
        
        if (control_mode_switch_duration > LONG_MODE_SWITCH_PRESS)
        {
            Serial.println("long press - set time");
        }
        else
        {
            Serial.println("short press - show weather");
        }
    }
}
