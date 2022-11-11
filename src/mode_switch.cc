
#include <Arduino.h>
#include <PinChangeInterrupt.h>
#include <RTClib.h>

#include "mode_switch.h"

#define SWITCH_INTERVAL 150  // ms
#define SWITCH_PRESS_2S 2000 // 2 Seconds
#define SWITCH_PRESS_5S 5000 // 5 S

volatile unsigned int mode_switch_time = 0;
volatile unsigned int mode_switch_duration = 0;

volatile unsigned int input_switch_time = 0;
volatile unsigned int input_switch_duration = 0;

volatile enum modes mode = main;
volatile enum main_modes main_mode = show_time;
volatile enum set_time_modes set_time_mode = set_hours;

extern volatile int digit_0;
extern volatile int digit_1;
extern volatile int digit_2;
extern volatile int digit_3;
extern volatile int digit_4;
extern volatile int digit_5;

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

/*
   set_hours,
    adv_hours_slow, // no fast advance for hours

    set_minutes,
    adv_minutes_slow,
    adv_minutes_fast,

    zero_seconds
    */
void set_time_mode_next() {
    switch (set_time_mode) {
    case set_hours:
    case adv_hours_slow:
        set_time_mode = set_minutes;
        break;

    case set_minutes:
    case adv_minutes_slow:
    case adv_minutes_fast:
        set_time_mode = zero_seconds;
        break;

    case zero_seconds:
        set_time_mode = set_hours;
        break;

    default:
        break;
    }
}

// Set by the 1 Hz interrupt
extern volatile byte tick;

// This is clock's time, updated when set-time mode is exited
extern DateTime dt;

#if USE_DS3231
extern RTC_DS3231 rtc;
#elif USE_DS1307
extern RTC_DS1307 rtc;
#else
#error "Must define one of DS3231 or DS1307"
#endif

DateTime new_dt; // initialized to 'dt' when set_time mode is entered

volatile unsigned int set_time_mode_handler_prev = 0;
volatile unsigned int set_time_mode_handler_time = 0;
volatile unsigned int set_time_mode_handler_duration = 0;

// Called from the main loop frequently
void set_time_mode_handler() {
    digit_0 = new_dt.second() % 10;
    digit_1 = new_dt.second() / 10;

    digit_2 = new_dt.minute() % 10;
    digit_3 = new_dt.minute() / 10;

    digit_4 = new_dt.hour() % 10;
    digit_5 = new_dt.hour() / 10;

#if 1
    set_time_mode_handler_prev = set_time_mode_handler_time;
    set_time_mode_handler_time = millis();
    set_time_mode_handler_duration = (set_time_mode_handler_prev != 0)
                                         ? set_time_mode_handler_time - set_time_mode_handler_prev
                                         : 0;

    static int tick_count = 0;
    cli(); // Protect 'tick' against update while in use
    if (tick) {
        tick = LOW;
        tick_count++;
    }
    sei();

    switch (set_time_mode) {
    case set_hours:
    case adv_hours_slow:

        break;

    case set_minutes:
    case adv_minutes_slow:
    case adv_minutes_fast:

        break;

    case zero_seconds:
        rtc.adjust(new_dt);
        dt = rtc.now();
        break;

    default:
        break;
    }
#endif
}

// Called by an interrupt handler - no I2C
// TODO Change that so that set_time_mode_handler() calls this and the zero_seconds
//  state calls adjust().
void set_time_mode_advance_by_one() {
    switch (set_time_mode) {
    case set_hours:
    case adv_hours_slow: {
        if (new_dt.hour() == 23) {
            new_dt = DateTime(new_dt.year(), new_dt.month(), new_dt.day(), 0, new_dt.minute(), new_dt.second());
        } else {
            uint8_t hour = new_dt.hour() + 1;
            new_dt = DateTime(new_dt.year(), new_dt.month(), new_dt.day(), hour, new_dt.minute(), new_dt.second());
        }
        break;
    }

    case set_minutes:
    case adv_minutes_slow:
    case adv_minutes_fast: {
        if (new_dt.minute() == 59) {
            new_dt = DateTime(new_dt.year(), new_dt.month(), new_dt.day(), new_dt.hour(), 0, new_dt.second());
        } else {
            uint8_t minute = new_dt.minute() + 1;
            new_dt = DateTime(new_dt.year(), new_dt.month(), new_dt.day(), new_dt.hour(), minute, new_dt.second());
        }
        break;
    }

    case zero_seconds:
        new_dt = DateTime(new_dt.year(), new_dt.month(), new_dt.day(), new_dt.hour(), new_dt.minute(), 0);
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
        mode_switch_duration = 0;
        mode_switch_time = interrupt_time;
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
        mode_switch_duration = interrupt_time - mode_switch_time;
        mode_switch_time = interrupt_time;

        // In the 'main' mode, a short press changes to set-time mode. A long press
        // does nothing.
        //
        // In the set_time mode, a short press cycles the set_time modes. A long press
        // returns to the main mode
        if (mode_switch_duration > SWITCH_PRESS_5S) {
            // noop for now
            Serial.println("very long press: ?");
        } else if (mode_switch_duration > SWITCH_PRESS_2S) {
            Serial.print("long press: ");
            if (mode == main) {
                Serial.println("set time");
                mode = set_time;
                set_time_mode = set_hours;

                new_dt = dt; // initialize the new DateTime object to now
            } else if (mode == set_time) {
                Serial.println("main");
                mode = main;
            } else {
                Serial.println("?");
            }
        } else {
            Serial.print("short press: ");
            if (mode == main) {
                main_mode_next();
                Serial.print("Main mode ");
                Serial.println(main_mode);
            } else if (mode == set_time) {
                set_time_mode_next();
                Serial.print("set_time mode ");
                Serial.println(set_time_mode);
            } else {
                Serial.println("?");
            }
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
#if 1
        if (mode == main) {
            Serial.print("Main mode: ");
            Serial.println(main_mode);
        } else if (mode == set_time) {
            Serial.print("Set time mode: ");
            Serial.println(set_time_mode);
            set_time_mode_advance_by_one();
        }
#endif
    }

    last_interrupt_time = interrupt_time;
}
