
#include <Arduino.h>

#include "mode_switch.h"

#define SWITCH_INTERVAL 100 // ms
#define LONG_MODE_SWITCH_PRESS 2000

volatile unsigned int mode_switch_time = 0;
volatile unsigned int mode_switch_duration = 0;

volatile unsigned int input_switch_time = 0;
volatile unsigned int input_switch_duration = 0;

// The next two functions are ISRs that implement the mode-switch.
// Forward declaration
void mode_switch_release();

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
void mode_switch_push()
{
    if (millis() > mode_switch_time + SWITCH_INTERVAL)
    {
        Serial.println("mode switch push");
        // Triggered on the rising edge is the button press; start the timer
        mode_switch_time = millis();
        mode_switch_duration = 0;
        attachInterrupt(digitalPinToInterrupt(MODE_SWITCH), mode_switch_release, RISING);
    }
}

/**
 * When the mode switch is released, this ISR is run.
 */
void mode_switch_release()
{
    if (millis() > mode_switch_time + SWITCH_INTERVAL)
    {
        Serial.println("mode switch release");
        attachInterrupt(digitalPinToInterrupt(MODE_SWITCH), mode_switch_push, FALLING);
        mode_switch_duration = millis() - mode_switch_time;
        mode_switch_time = millis();

        if (mode_switch_duration > LONG_MODE_SWITCH_PRESS)
        {
            Serial.println("long press - set time");
        }
        else
        {
            Serial.println("short press - show weather");
        }
    }
}

void input_switch_push()
{
    if (millis() > input_switch_time + SWITCH_INTERVAL)
    {
        Serial.println("input switch press");
        // Triggered on the rising edge is the button press; start the timer
        input_switch_time = millis();
        input_switch_duration = 0;
        attachInterrupt(digitalPinToInterrupt(MODE_SWITCH), input_switch_release, RISING);
    }
}

void input_switch_release()
{
    if (millis() > input_switch_time + SWITCH_INTERVAL)
    {
        Serial.println("input switch release");
        attachInterrupt(digitalPinToInterrupt(MODE_SWITCH), input_switch_push, FALLING);
        input_switch_duration = millis() - input_switch_time;
        input_switch_time = millis();

        Serial.print("Duration: ");
        Serial.println(input_switch_duration);
    }
}
