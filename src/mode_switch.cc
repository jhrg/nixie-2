
#include <Arduino.h>
#include <PinChangeInterrupt.h>

#include "mode_switch.h"

#define SWITCH_INTERVAL 150 // ms
#define LONG_MODE_SWITCH_PRESS 2000

volatile unsigned int mode_switch_time = 0;
volatile unsigned int mode_switch_duration = 0;

volatile unsigned int input_switch_time = 0;
volatile unsigned int input_switch_duration = 0;

volatile enum modes modes = main;
volatile enum main_mode main_mode = show_time;

void main_mode_next() {
    switch (main_mode) {
    case show_time:
        main_mode = show_date;
        break;

    case show_date:
        main_mode = show_weather;
        break;

    case show_weather:
        main_mode = show_time;
        break;

    default:
        break;
    }
}

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
void mode_switch_push() {
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();

    if (interrupt_time - last_interrupt_time > SWITCH_INTERVAL) {
        Serial.println("mode switch push");
        // Triggered on the rising edge is the button press; start the timer
        mode_switch_time = millis();
        mode_switch_duration = 0;
        attachInterrupt(digitalPinToInterrupt(MODE_SWITCH), mode_switch_release, RISING);
    }

    last_interrupt_time = interrupt_time;
}

/**
 * When the mode switch is released, this ISR is run.
 */
void mode_switch_release() {
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();

    if (interrupt_time - last_interrupt_time > SWITCH_INTERVAL) {
        Serial.println("mode switch release");
        attachInterrupt(digitalPinToInterrupt(MODE_SWITCH), mode_switch_push, FALLING);
        mode_switch_duration = millis() - mode_switch_time;
        mode_switch_time = millis();

        if (mode_switch_duration > LONG_MODE_SWITCH_PRESS) {
            Serial.println("long press - set time");
        } else {
            Serial.println("short press - show weather");
        }
    }

    last_interrupt_time = interrupt_time;
}

void input_switch_push() {
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();

    if (interrupt_time - last_interrupt_time > SWITCH_INTERVAL) {
        Serial.print("input switch press, ");
        input_switch_time = interrupt_time;
        input_switch_duration = 0;

        attachPCINT(digitalPinToPCINT(INPUT_SWITCH), input_switch_release, RISING);

        main_mode_next();
        Serial.print("Main mode: ");
        Serial.println(main_mode);
    }

    last_interrupt_time = interrupt_time;
}

void input_switch_release() {
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();

    if (interrupt_time - last_interrupt_time > SWITCH_INTERVAL) {
        Serial.print("input switch release, ");

        attachPCINT(digitalPinToPCINT(INPUT_SWITCH), input_switch_push, FALLING);
        input_switch_duration = interrupt_time - input_switch_time;

        Serial.print("Duration: ");
        Serial.println(input_switch_duration);
    }

    last_interrupt_time = interrupt_time;
}
