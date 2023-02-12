
#include <Arduino.h>
#include <PinChangeInterrupt.h>
#include <RTClib.h>

#include "mode_switch.h"
#include "print.h"

#define SWITCH_INTERVAL 150     // ms
#define SWITCH_PRESS_2S 2000    // 2 Seconds
#define SWITCH_PRESS_5S 5000    // 5 S
#define SWITCH_PRESS_10S 10000  // 10 S

extern volatile int digit_0;
extern volatile int digit_1;
extern volatile int digit_2;
extern volatile int digit_3;
extern volatile int digit_4;
extern volatile int digit_5;

extern volatile int d0_rhdp;
extern volatile int d1_rhdp;
extern volatile int d2_rhdp;
extern volatile int d3_rhdp;
extern volatile int d4_rhdp;
extern volatile int d5_rhdp;

// These control the brightness of the three digit pairs so that the pair being
// set is bright and the other two are dim.
extern volatile bool pair_0;
extern volatile bool pair_1;
extern volatile bool pair_2;

extern DateTime dt;  // This is clock's time, updated when set-time mode is exited
DateTime new_dt;     // initialized to 'dt' when set_time mode is entered
extern RTC_DS3231 rtc;
extern int brightness;

extern void print_digits(bool newline);
extern void blank_dp();

volatile enum switch_press_duration mode_switch_press = none;

volatile unsigned long input_switch_down_time = 0;
volatile enum switch_press_duration input_switch_press = none;

volatile bool input_switch_down = false;  // set to true by the IRQ
volatile bool input_switch_up = true;

volatile enum modes mode = main;

volatile enum main_modes main_mode = show_time;
volatile enum set_time_modes set_time_mode = set_month;

/**
 * These three select one of the three pairs of digits so they can be highlighted,
 * flashed, etc., while the clock's time, date, etc., is being set.
 */
void set_pair_2() {
    pair_2 = true;
    pair_0 = pair_1 = false;
}
void set_pair_1() {
    pair_1 = true;
    pair_0 = pair_2 = false;
}
void set_pair_0() {
    pair_0 = true;
    pair_1 = pair_2 = false;
}
void set_pair_all() {
    pair_0 = pair_1 = pair_2 = true;
}

extern volatile int weather_display_state;

/**
 * Cycle the main modes
 */
void main_mode_next() {
    cli();

    switch (main_mode) {
        case show_time:
            main_mode = show_date;
            break;

        case show_date:
            main_mode = show_weather;
            weather_display_state = 0;
            break;

        case show_weather:
            main_mode = show_time;
            break;

        default:
            break;
    }

    sei();
}

/**
 * @brief Advance the set_time_mode state machine
 * @note the order is month -> day -> year -> hour -> min -> zero seconds
 */
void set_date_time_mode_next() {
    cli();

    switch (set_time_mode) {
        case set_month:
            set_time_mode = set_day;
            blank_dp();
            d3_rhdp = 1;
            d2_rhdp = 1;
            set_pair_1();
            break;

        case set_day:
            set_time_mode = set_year;
            blank_dp();
            d1_rhdp = 1;
            d0_rhdp = 1;
            set_pair_0();
            break;

        case set_year:
            set_time_mode = set_hour;
            blank_dp();
            d5_rhdp = 1;
            d4_rhdp = 1;
            set_pair_2();
            break;

        case set_hour:
            set_time_mode = set_minute;
            blank_dp();
            d3_rhdp = 1;
            d2_rhdp = 1;
            set_pair_1();
            break;

        case set_minute:
            set_time_mode = zero_seconds;
            blank_dp();
            d1_rhdp = 1;
            d0_rhdp = 1;
            set_pair_0();
            break;

        case zero_seconds:
            set_time_mode = set_month;
            blank_dp();
            d5_rhdp = 1;
            d4_rhdp = 1;
            set_pair_2();
            break;

        default:
            break;
    }

    sei();
}

void set_date_time_mode_advance_by_one() {
    switch (set_time_mode) {
        case set_month: {
            if (new_dt.month() == 12) {
                new_dt = DateTime(new_dt.year(), 1, new_dt.day(), new_dt.hour(), new_dt.minute(), new_dt.second());
            } else {
                new_dt = DateTime(new_dt.year(), new_dt.month() + 1, new_dt.day(), new_dt.hour(), new_dt.minute(), new_dt.second());
            }
            break;
        }

        case set_day: {
            if (new_dt.day() == 31) {
                new_dt = DateTime(new_dt.year(), new_dt.month(), 1, new_dt.hour(), new_dt.minute(), new_dt.second());
            } else {
                new_dt = DateTime(new_dt.year(), new_dt.month(), new_dt.day() + 1, new_dt.hour(), new_dt.minute(), new_dt.second());
            }
            break;
        }

        case set_year: {
            if (new_dt.year() == 2099) {
                new_dt = DateTime(2000, new_dt.month(), new_dt.day(), new_dt.hour(), new_dt.minute(), new_dt.second());
            } else {
                new_dt = DateTime(new_dt.year() + 1, new_dt.month(), new_dt.day(), new_dt.hour(), new_dt.minute(), new_dt.second());
            }
            break;
        }

        case set_hour: {
            if (new_dt.hour() == 23) {
                new_dt = DateTime(new_dt.year(), new_dt.month(), new_dt.day(), 0, new_dt.minute(), new_dt.second());
            } else {
                uint8_t hour = new_dt.hour() + 1;
                new_dt = DateTime(new_dt.year(), new_dt.month(), new_dt.day(), hour, new_dt.minute(), new_dt.second());
            }
            break;
        }

        case set_minute: {
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
            rtc.adjust(new_dt);  // Set the clock to the new time
            dt = rtc.now();      // Update the global variable that holds the time
            break;

        default:
            break;
    }
}

#define ADVANCE_REALLY_FAST 100  // 100ms
#define ADVANCE_FAST 500
#define ADVANCE 1000

// Called from the main loop frequently when in the set_date_time mode
void set_date_time_mode_handler() {
    switch (set_time_mode) {
        case set_year:
        case set_month:
        case set_day:
            digit_0 = new_dt.year() % 10;
            digit_1 = (new_dt.year() - 2000) / 10;

            digit_2 = new_dt.day() % 10;
            digit_3 = new_dt.day() / 10;

            digit_4 = new_dt.month() % 10;
            digit_5 = new_dt.month() / 10;
            break;

        case set_hour:
        case set_minute:
        case zero_seconds:
            digit_0 = new_dt.second() % 10;
            digit_1 = new_dt.second() / 10;

            digit_2 = new_dt.minute() % 10;
            digit_3 = new_dt.minute() / 10;

            digit_4 = new_dt.hour() % 10;
            digit_5 = new_dt.hour() / 10;
            break;
    }
#if 0
    // This provides a way to track how long the switch is being held down and thus how
    // frequently to call set_time_mode_advance_by_one().
    static unsigned long last_input_call_time = 0;

    // if the switch was not held down for 2s or longer. This keeps the count
    // from being incremented and extra time when a long press is released
    if (input_switch_up && input_switch_press == quick) {
        set_date_time_mode_advance_by_one();
    } else if (input_switch_down && !input_switch_up) {
        unsigned long duration = millis() - input_switch_down_time;
        if (duration > SWITCH_PRESS_10S) {
            // call set_time_mode_advance_by_one() really often (1/10th a second)
            if ((last_input_call_time == 0) || (millis() - last_input_call_time > ADVANCE_REALLY_FAST)) {
                DPRINT("Really fast\n");
                set_date_time_mode_advance_by_one();
                last_input_call_time = millis();
            }
        } else if (duration > SWITCH_PRESS_5S) {
            // call set_time_mode_advance_by_one() often (half second)
            if ((last_input_call_time == 0) || (millis() - last_input_call_time > ADVANCE_FAST)) {
                DPRINT("fast\n");
                set_date_time_mode_advance_by_one();
                last_input_call_time = millis();
            }
        } else if (duration > SWITCH_PRESS_2S) {
            // call set_time_mode_advance_by_one() once per second
            if ((last_input_call_time == 0) || (millis() - last_input_call_time > ADVANCE)) {
                DPRINT("regular\n");
                set_date_time_mode_advance_by_one();
                last_input_call_time = millis();
            }
        }
    }

    // If the input switch was released, reset the state and the also last_input_call_time
    if (input_switch_up) {
        input_switch_up = false;        // input_switch_released is only reset in this function
        last_input_call_time = 0;       // last_input... is static local to this function
    }
#endif
}

void clear_set_time_mode_state_variables() {
    input_switch_down_time = 0;
    // input_switch_duration = 0;

    input_switch_down = false;  // set to true by the IRQ
    input_switch_up = true;
}

/**
 * Return true if the mode switch was pressed and released.
 *
 * This does not change the state of the mode switch state variables.
 */
bool poll_mode_button() {
#if DEBUG
    static enum switch_press_duration last_press = none;
    if (last_press != mode_switch_press) {
        print("Mode switch state: %d\n", mode_switch_press);
        last_press = mode_switch_press;
    }
#endif
    return mode_switch_press != none;
}

/**
 * Get the state of the mode switch.
 */
enum switch_press_duration get_mode_button() {
    return mode_switch_press;
}

/**
 * Reset the mode button state
 */
void reset_mode_button() {
    mode_switch_press = none;
}

void mode_switch_medium_press() {
    DPRINT("medium press: ");
    if (mode == main) {     // change mode to set_time_mode
        DPRINT("set time\n");
        mode = set_date_time;
        new_dt = dt;  // initialize the new DateTime object to now

        set_time_mode = set_month;

        blank_dp();  // Highlight digits to be set
        d5_rhdp = 1;
        d4_rhdp = 1;

        set_pair_2();        
    } else if (mode == set_date_time) {     // change mode to te main mode
        DPRINT("main\n");
        blank_dp();
        set_pair_all();
        clear_set_time_mode_state_variables();
        mode = main;
    } 
}

void mode_switch_quick_press() {
    DPRINT("short press: ");
    if (mode == main) {
        main_mode_next();
        blank_dp();  // Added for quick change from weather to date
        DPRINTV("Main mode %d\n", main_mode);
    } else if (mode == set_date_time) {
        set_date_time_mode_next();
        DPRINTV("set_time mode %d\n", set_time_mode);
    } 
}

/**
 * Assume that poll_mode_switch() returns true.
 * 
 * Use the various mode switch and mode state to perform the correct
 * action. Then reset the mode switch state to none.
*/
void process_mode_switch_press() {
    switch (get_mode_button()) {
        case none:
            return;
        case very_long:
            break;
        case medium:
            mode_switch_medium_press();
            break;
        case quick:
            mode_switch_quick_press();
            break;
    }
    reset_mode_button();
}

volatile unsigned long mode_switch_down_time = 0;  // Used by the two mode switch ISRs

/**
 * First ISR for the mode switch. Triggered when the switch is pressed.
 * The mode switch GPIO is held LOW normally and a button press causes
 * the input to go HIGH. The ISR is triggered on the rising edge of
 * the interrupt. Capture the time and set the duration to zero. Then
 * register a second ISR for the button release, which will be triggered
 * when the GPIO pin state goes back to the LOW level.
 *
 * @note Interrupts are disabled in ISR functions and millis() does
 * not advance. Thus, the value of millis() will be the value just before
 * entering this ISR.
 *
 * @note The ISR uses SWITCH_INTERVAL (150 ms) to debounce the switch.
 */
void mode_switch_push() {
    static unsigned long last_interrupt_time = 0;
    unsigned long now = millis();

    if (now - last_interrupt_time > SWITCH_INTERVAL) {
        mode_switch_down_time = now;
        mode_switch_press = none;  // This is set when the switch is released
        attachPCINT(digitalPinToPCINT(MODE_SWITCH), mode_switch_release, FALLING);
    }

    last_interrupt_time = now;
}

/**
 * When the mode switch is released, this ISR is run.
 */
void mode_switch_release() {
    static unsigned long last_interrupt_time = 0;
    unsigned long now = millis();

    if (now - last_interrupt_time > SWITCH_INTERVAL) {
        // TODO take into account millis() rolling over. 2/11/23
        unsigned long mode_switch_duration = now - mode_switch_down_time;
        mode_switch_down_time = now;
        attachPCINT(digitalPinToPCINT(MODE_SWITCH), mode_switch_push, RISING);

        if (mode_switch_duration > SWITCH_PRESS_5S) {
            mode_switch_press = very_long;
        } else if (mode_switch_duration > SWITCH_PRESS_2S) {
            mode_switch_press = medium;
        } else {
            mode_switch_press = quick;
        }
    }

    last_interrupt_time = now;
}

/**
 * Return true if the input switch was pressed and released.
 *
 * This does not change the state of the input switch state variables.
 */
bool poll_input_button() {
#if DEBUG
    static enum switch_press_duration last_press = none;
    if (last_press != input_switch_press) {
        print("Mode switch state: %d\n", input_switch_press);
        last_press = input_switch_press;
    }
#endif
    return input_switch_press != none;
}

bool input_switch_held_down() {
    return input_switch_down && !input_switch_up;
}

/**
 * Get the state of the input switch.
 */
enum switch_press_duration get_input_button() {
    return input_switch_press;
}

/**
 * Reset the input button state
 */
void reset_input_button() {
    input_switch_press = none;
}

void input_switch_quick_press() {
    if (mode == main) {
        // brightness is an unsigned char
        brightness = (brightness == 5) ? 0 : brightness + 1;
    } else if (mode == set_date_time) {
        set_date_time_mode_advance_by_one();
    }
}

/**
 * Assume that poll_input_switch() returns true.
 *
 * Use the various input switch and input state to perform the correct
 * action. Then reset the input switch state to none.
 */
void process_input_switch_press() {
    switch (get_input_button()) {
        case none:
            return;
        case very_long:
        case medium:
            break;
        case quick:
            input_switch_quick_press();
            break;
    }
    reset_input_button();
}

void process_input_switch_held() {
    // This provides a way to track how long the switch is being held down and thus how
    // frequently to call set_time_mode_advance_by_one().
    static unsigned long last_input_call_time = 0;

    cli();  // Prevent input_switch_down_time from being zeroed

    if (!input_switch_held_down())  // Makes sure it's still held down
        return;

    unsigned long duration = millis() - input_switch_down_time;

    sei();

    if (duration > SWITCH_PRESS_10S) {
        // call set_time_mode_advance_by_one() really often (1/10th a second)
        if ((last_input_call_time == 0) || (millis() - last_input_call_time > ADVANCE_REALLY_FAST)) {
            DPRINT("Really fast\n");
            set_date_time_mode_advance_by_one();
            last_input_call_time = millis();
        }
    } else if (duration > SWITCH_PRESS_5S) {
        // call set_time_mode_advance_by_one() often (half second)
        if ((last_input_call_time == 0) || (millis() - last_input_call_time > ADVANCE_FAST)) {
            DPRINT("fast\n");
            set_date_time_mode_advance_by_one();
            last_input_call_time = millis();
        }
    } else if (duration > SWITCH_PRESS_2S) {
        // call set_time_mode_advance_by_one() once per second
        if ((last_input_call_time == 0) || (millis() - last_input_call_time > ADVANCE)) {
            DPRINT("regular\n");
            set_date_time_mode_advance_by_one();
            last_input_call_time = millis();
        }
    }
}

// These two IRQ handlers for the input switch share state information about
// the time the switch was pressed, duration of the press and if the switch
// is currently pressed or has been released.
//
// input_switch_press & input_switch_released describe the state of the button
// input_switch_time is the time in ms that the switch was pressed
// input_switch_duration is the time span between press and release
//
void input_switch_push() {
    static unsigned long last_interrupt_time = 0;
    unsigned long now = millis();

    if (now - last_interrupt_time > SWITCH_INTERVAL) {
        // Use these to tell if the switch is being held down
        input_switch_down = true;
        input_switch_up = false;

        input_switch_down_time = now;  // Use this to tell how long it has been held down
        input_switch_press = none;     // This is set when the switch is released
        attachPCINT(digitalPinToPCINT(INPUT_SWITCH), input_switch_release, FALLING);
    }

    last_interrupt_time = now;
}

void input_switch_release() {
    static unsigned long last_interrupt_time = 0;
    unsigned long now = millis();

    if (now - last_interrupt_time > SWITCH_INTERVAL) {
        unsigned long input_switch_duration = now - input_switch_down_time;

        input_switch_down_time = 0;  // TODO set to zero in the mode switch code above, too

        input_switch_down = false;
        input_switch_up = true;  // reset above in set_date_time_mode_handler()

        attachPCINT(digitalPinToPCINT(INPUT_SWITCH), input_switch_push, RISING);

        if (input_switch_duration > SWITCH_PRESS_5S) {
            input_switch_press = very_long;
        } else if (input_switch_duration > SWITCH_PRESS_2S) {
            input_switch_press = medium;
        } else {
            input_switch_press = quick;
        }
    }

    last_interrupt_time = now;
}
