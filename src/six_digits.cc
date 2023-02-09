/*
  Six digit clock.

  10/1/22 jhrg
*/

#include <Arduino.h>
#include <PinChangeInterrupt.h>
#include <RTClib.h> // https://github.com/adafruit/RTClib

#include "hv_ps.h"
#include "met_sensor.h"
#include "mode_switch.h"
#include "ambient_light.h"
#include "print.h"

extern volatile enum modes mode;
extern volatile enum main_modes main_mode;

#define BAUD_RATE 115200        // old: 9600
#define CLOCK_QUERY_INTERVAL 12 // seconds

#define CLOCK_1HZ 2 // D2

// This is PORTC (bits 0 to 3; 4 & 5 are for the I2C bus)
#define BCD_A A0
#define BCD_B A1
#define BCD_C A2
#define BCD_D A3

// BCD codes for the digits 0 to 9. Only uses the low nyble
uint8_t bcd[10] = {
    B00000000,
    B00000001,
    B00000010,
    B00000011,
    B00000100,
    B00000101,
    B00000110,
    B00000111,
    B00001000,
    B00001001};

// PORTD, the decimal points and digit 0
#define RHDP B01000000    // D6 (Port D)
#define DIGIT_0 B10000000 // Pin 7 (Port D)

// All of the remaining digits are on PORTB (D8 - D15).
#define DIGIT_1 B00000001 // Pin 8 (Port B)
#define DIGIT_2 B00000010 // Pin 9
#define DIGIT_3 B00001000 // Pin 11
#define DIGIT_4 B00010000 // Pin 12
#define DIGIT_5 B00100000 // Pin 13

RTC_DS3231 rtc;
extern Adafruit_BME280 bme;

// The state machine for the display multiplexing
volatile bool blanking;
volatile int digit;

// The current display digits
volatile int digit_0;
volatile int digit_1;
volatile int digit_2;
volatile int digit_3;
volatile int digit_4;
volatile int digit_5;

volatile int d0_rhdp;
volatile int d1_rhdp;
volatile int d2_rhdp;
volatile int d3_rhdp;
volatile int d4_rhdp;
volatile int d5_rhdp;

/**
 * Print the values of the current digits
 */
void print_digits(bool newline) {
#if DEBUG
    print("%01d-%01d-%01d-%01d-%01d-%01d\n", digit_5, digit_4, digit_3, digit_2, digit_1, digit_0);
#endif
}

/**
 * Print the current time, formatted
 */
void print_time(DateTime dt, bool print_newline = false) {
#if DEBUG
    // or Serial.println(now.toString(buffer));, buffer == YY/MM/DD hh:mm:ss
    print("%02d/%02d/%02d %02d:%02d:%02d", dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
    if (print_newline)
        print("\n");
#endif
}

// The global value of time - enables advancing time without I2C use. This
// is global so the value set in setup() will be available initially in the loop().
//
// DateTime cannot be 'volatile' given its definition
DateTime dt;

void update_display_with_time() {
    digit_0 = dt.second() % 10;
    digit_1 = dt.second() / 10;

    digit_2 = dt.minute() % 10;
    digit_3 = dt.minute() / 10;

    digit_4 = dt.hour() % 10;
    digit_5 = dt.hour() / 10;
}

// mm/dd/yy
void update_display_with_date() {
    digit_0 = dt.year() % 10;
    digit_1 = (dt.year() - 2000) / 10;

    digit_2 = dt.day() % 10;
    digit_3 = dt.day() / 10;

    digit_4 = dt.month() % 10;
    digit_5 = dt.month() / 10;
}

void update_display_using_mode() {
    static bool blinker = false; // blink dp for date

    switch (main_mode) {
    case show_time:
        update_display_with_time();
        break;

    case show_date:
        blinker = !blinker;
        d0_rhdp = blinker ? 1 : 0;
        update_display_with_date();
#if DEBUG
        print_digits(true);
#endif
        break;

    case show_weather: {
        update_display_with_weather();
        break;
    }

    default:
        break;
    }
}

void blank_dp() {
    d0_rhdp = 0;
    d1_rhdp = 0;
    d2_rhdp = 0;
    d3_rhdp = 0;
    d4_rhdp = 0;
    d5_rhdp = 0;
}

// Set HIGH when the CLOCK_QUERY_INTERVAL seconds have elapsed and
// the RTC should be accessed.
volatile bool get_time = false;

// Set HIGH when the 1 second interrupt been triggered by the clock,
// Used in main_mode_handler()
volatile bool update_display = false;

/**
 * @brief Record that one second has elapsed
 */
void timer_1HZ_tick_ISR() {
    static volatile int tick_count = 0;

    tick_count++;

    if (tick_count >= CLOCK_QUERY_INTERVAL) {
        // update time using I2C access to the clock
        tick_count = 0;
        get_time = true;
    }

    // when get_time is false, main_mode_handler() adds one second to the global
    // time (dt) when update_display is true.
    update_display = true;
}

/**
 * Set brightness by varying the digit on and off time.
 *
 * 900us, ..., 0+ with the off time being 100us, ..., 1000us
 * With the pre-scalar at 64, a count of 0 is 4uS, 1 is 8uS, ...,
 * For example: 224 = [(16*10^6 / 64 ) * 0.000 900] - 1; (must be <256)
 *
 * There are six brightness levels 0-5 and the counter values for the on and
 * off times are precomputed and used via the arrays below. The brightness
 * level is held in the following global that can be set by the input button
 * (see set_mode.cc). The default/initial value is 0 which is the brightest
 * level.
 */
int brightness = 0;

int brightness_count[] = {231, 179, 128, 76, 24, 10}; // ~900us, ...
int blanking_count[] = {24, 76, 127, 179, 231, 245}; // 100us, ...

/**
 * @brief The display multiplexing code. A simple state-machine
 *
 * The state variable blanking describes the current state, so when blanking,
 * light a digit and make the state non-blanking.
 */
ISR(TIMER2_COMPA_vect) {
    // See https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html
#if TIMER_INTERRUPT_DIAGNOSTIC
    PORTD |= _BV(PORTD3);
#endif

    // If the current state is blanking, stop blanking and enter digit display state
    // From the scope, when blanking is true, time in this code is 5us
    if (blanking) {
        switch (digit) {
        case 0:
            //  Set the BCD value on A0-A3. Preserve the values of A4-A7
            PORTC &= B11110000;

            // use a digit value of -1 for blanking
            if (digit_0 > -1) {
                PORTC |= bcd[digit_0];

                // Turn on the digit, The digits are blanked below during the blanking state
                PORTD |= DIGIT_0;

                // Turn on the decimal point(s) if set
                if (d0_rhdp)
                    PORTD |= RHDP;
            }
            // move the state to the next digit
            digit += 1;
            break;

        case 1:
            PORTC &= B11110000;
            if (digit_1 > -1) {
                PORTC |= bcd[digit_1];
                PORTB |= DIGIT_1;
                if (d1_rhdp)
                    PORTD |= RHDP;
            }
            digit += 1;
            break;

        case 2:
            PORTC &= B11110000;
            if (digit_2 > -1) {
                PORTC |= bcd[digit_2];
                PORTB |= DIGIT_2;
                if (d2_rhdp)
                    PORTD |= RHDP;
            }

            digit += 1;
            break;

        case 3:
            PORTC &= B11110000;
            if (digit_3 > -1) {
                PORTC |= bcd[digit_3];
                PORTB |= DIGIT_3;
                if (d3_rhdp)
                    PORTD |= RHDP;
            }
            digit += 1;
            break;

        case 4:
            PORTC &= B11110000;
            if (digit_4 > -1) {
                PORTC |= bcd[digit_4];
                PORTB |= DIGIT_4;
                if (d4_rhdp)
                    PORTD |= RHDP;
            }
            digit += 1;
            break;

        case 5:
            PORTC &= B11110000;
            if (digit_5 > -1) {
                PORTC |= bcd[digit_5];
                PORTB |= DIGIT_5;
                if (d5_rhdp)
                    PORTD |= RHDP;
            }
            digit = 0;
            break;
        }

        // State is not blanking
        blanking = false;

        // Set the timer to illuminate the digit (e.g., for 900uS)
        OCR2A = brightness_count[brightness];
    } else {
        // blank_display
        PORTB &= B11000100;
        PORTD &= B00111111;

        // State is blanking
        blanking = true;

        // Set the timer to blank for, e.g., 100uS. See above
        OCR2A = blanking_count[brightness];
    }

#if TIMER_INTERRUPT_DIAGNOSTIC
    PORTD &= ~_BV(PORTD3);
#endif
}

void setup() {
// Enable some minor, chatty, messages on the serial line.
#if DEBUG
    Serial.begin(BAUD_RATE);
#endif
    DPRINT("boot\n");

    // Initialize all I/O pins to output, then set up the inputs/interrupts
    DDRD = B11111111; // D0 - D7
    DDRC = B00111111; // A0 - A5, bit 6 is RST, 7 is undefined
    DDRB = B00111111; // D8 - D13, bits 6,7 are for the crystal

    // Initialize all GPIO pins to LOW
    PORTD = B00000000;
    PORTC = B00000000;
    PORTB = B00000000;

    Wire.begin();

    if (rtc.begin()) {
        DPRINT("DS3131 RTC Start\n");
    } else {
        DPRINT("Couldn't find RTC\n");
        // TODO Set error flag
    }

    if (init_bme280()) {
        DPRINT("BME280 Start\n");
    } else {
        DPRINT("Couldn't setup BME280\n");
        // TODO Set error flag
    }

#if ADJUST_TIME
    // Run this here, before serial configuration to shorten the delay
    // between the compiled-in times and the set operation.
    Serial.print("Build date: ");
    Serial.println(__DATE__);
    Serial.print("Build time: ");
    Serial.println(__TIME__);

    DateTime build_time = DateTime(F(__DATE__), F(__TIME__));
    TimeSpan ts(ADJUST_TIME);
    build_time = build_time + ts;
    DateTime now = rtc.now();

    Serial.print(now.unixtime());
    Serial.print(", ");
    Serial.println(build_time.unixtime());

    if (abs(now.unixtime() - build_time.unixtime()) > 60) {
        Serial.print("Adjusting the time: ");
        print_time(build_time, true);

        rtc.adjust(build_time);
    }
#endif

#if USE_DS3231
    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
#elif USE_DS1307
    rtc.writeSqwPinMode(DS1307_SquareWave1HZ);
#endif

    dt = rtc.now();
    print_time(dt, true);

    // blank the display
    digit_0 = -1;
    digit_1 = -1;
    digit_2 = -1;
    digit_3 = -1;
    digit_4 = -1;
    digit_5 = -1;

    // These don't need to be set to zero to blank, the -1 digit value
    // blanks the whole tube. But the code might as well start out with
    // rational values.
    d0_rhdp = 0;
    d1_rhdp = 0;
    d2_rhdp = 0;
    d3_rhdp = 0;
    d4_rhdp = 0;
    d5_rhdp = 0;

    // State machine initial conditions:
    // start up as if the display has cycled once through already
    blanking = true;
    digit = 0; // FIXME Should be zero? 1/9/23

    ambient_light_setup();

    // Set up Timer2 so the PWN on pin 3 uses 32kHz. Also sets up the PID controller
    // parameters.
    hv_ps_setup();

    cli(); // stop interrupts

    // This is used for the 1Hz pulse from the clock that triggers
    // time updates.
    pinMode(CLOCK_1HZ, INPUT_PULLUP);

    // time_1Hz_tick() sets a flag that is tested in loop()
    attachInterrupt(digitalPinToInterrupt(CLOCK_1HZ), timer_1HZ_tick_ISR, RISING);

    // MODE_SWITCH is D3, external 1k pull down
    pinMode(MODE_SWITCH, INPUT);
    attachPCINT(digitalPinToPCINT(MODE_SWITCH), mode_switch_push, RISING);

    // INPUT_SWITCH is D4, external 1k pull down
    pinMode(INPUT_SWITCH, INPUT);
    attachPCINT(digitalPinToPCINT(INPUT_SWITCH), input_switch_push, RISING);

    // Set up timer 2 - controls the display multiplexing

    // set timer2 interrupt at 950uS. Toggles between 950 and 50 uS
    TCCR2A = 0; // set entire TCCR2A register to 0
    TCCR2B = 0; // same for TCCR0B
    TCNT2 = 0;  // initialize counter value to 0

    // set compare match register for 900uS increments
    OCR2A = brightness_count[0]; // = [(16*10^6 / 64 ) * 0.000 900] - 1; (must be <256)

    // turn on CTC mode
    TCCR2A |= _BV(WGM21); // (1 << WGM21)
    // Set CS22 bit for 64 pre-scaler --> B00000100
    TCCR2B |= _BV(CS22); // (1 << CS22)
    // enable timer compare interrupt
    TIMSK2 |= _BV(OCIE2A); // (1 << OCIE2A)

    sei(); // start interrupts
}

void main_mode_handler() {
    static TimeSpan ts(1); // a one-second time span

    if (get_time) {
        get_time = false;
        dt = rtc.now(); // This call takes about 1ms
        update_display_using_mode();
        ambient_light_adjust();
    } else if (update_display) {
        update_display = false;
        dt = dt + ts;                // Advance 'dt' by one second
        update_display_using_mode(); // true == adv time by 1s
    }
}

void loop() {
    hv_ps_adjust();

    if (mode == main) {
        main_mode_handler();
    } else if (mode == set_date_time) {
        set_date_time_mode_handler();
    }
}
