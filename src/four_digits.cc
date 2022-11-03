/*
  Four digit clock.

  10/1/22 jhrg
*/

#include <Arduino.h>

#include <PinChangeInterrupt.h>
#include <RTClib.h> // https://github.com/adafruit/RTClib

#include "mode_switch.h"
#include "DHT_sensor.h"

extern volatile enum modes modes;
extern volatile enum main_mode main_mode;

#define BAUD_RATE 9600
#define CLOCK_QUERY_INTERVAL 100 // seconds

#if TIMER_INTERRUPT_DIAGNOSTIC
// GPIO Pin 4, Port D; PORTB |= B0010000;
#define TIMER_INTERRUPT_TEST_PIN B0010000
#endif

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

// All of the digits are on PORTB (D8 - D15). This means there
// can be no SPI bus use.
#define DIGIT_0 B00000001 // D8
#define DIGIT_1 B00000010 // D9
#define DIGIT_2 B00000100 // D10
#define DIGIT_3 B00001000 // D11
#define DIGIT_4 B00010000 // D12
#define DIGIT_5 B00100000 // D13

#if USE_DS3231
RTC_DS3231 rtc;
#elif USE_DS1307
RTC_DS1307 rtc;
#else
#error "Must define one of DS3231 or DS1307"
#endif

// extern DHT_nonblocking dht;
// extern DHT_Unified dht;
// DHT_Unified dht(DHTPIN, DHTTYPE);

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

// time - enables advancing time without I2C use. This
// is global so the value set in setup() will be available
// initially in the loop().
//
// DateTime cannot be 'volatile' given its definition
DateTime dt;

void update_display_with_time()
{
    digit_0 = dt.second() % 10;
    digit_1 = dt.second() / 10;

    digit_2 = dt.minute() % 10;
    digit_3 = dt.minute() / 10;

    digit_4 = dt.hour() % 10;
    digit_5 = dt.hour() / 10;
}

// mm/dd/yy
void update_display_with_date()
{
    digit_0 = dt.year() % 10;
    digit_1 = (dt.year() - 2000) / 10;

    digit_2 = dt.day() % 10;
    digit_3 = dt.day() / 10;

    digit_4 = dt.month() % 10;
    digit_5 = dt.month() / 10;
}

// Set in the test_DHT code called by setup()
#if 0
unsigned long DHT_delay_ms = 0; // not used
#endif

void update_display_using_mode()
{
    switch (main_mode)
    {
    case show_time:
        update_display_with_time();
        break;

    case show_date:
        update_display_with_date();
        break;

    case show_weather:
        update_display_with_weather();
        break;

    default:
        break;
    }
}

void print_time(DateTime dt, bool print_newline = false)
{
    char str[64];
    snprintf(str, 64, "%02d-%02d-%02d %02d:%02d:%02d", dt.year(), dt.month(),
             dt.day(), dt.hour(), dt.minute(), dt.second());
    Serial.print(str);
    if (print_newline)
        Serial.println();
}

/**
 * Print the current time. Print get_time_duration if it is not zero
 * @param get_time_duration How long did the last get_time transaction take?
 */
void display_monitor_info(DateTime dt, uint32_t get_time_duration = 0)
{
    static unsigned int n = 0;
    Serial.print("Display: ");
    Serial.print(n++);
    Serial.print(", ");

    if (get_time_duration != 0)
    {
        print_time(dt, false);
        Serial.print(", I2C time query: ");
        Serial.print(get_time_duration);
        Serial.println(" uS");
    }
    else
    {
        print_time(dt, true);
    }
}

// Set HIGH when the 1 second interrupt been triggered by the clock
volatile byte tick = LOW;

/**
 * @brief Record that one second has elapsed
 */
void timer_1HZ_tick_ISR() {
    tick = HIGH;
}

/**
 * @brief The display multiplexing code. A simple state-machine
 */
ISR(TIMER2_COMPA_vect) {
    // TODO This might not be needed.
    // See https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html
#if TIMER_INTERRUPT_DIAGNOSTIC
    PORTD |= TIMER_INTERRUPT_TEST_PIN;
#endif

    // If the current state is blanking, stop blanking and enter digit display state
    if (blanking) {
        switch (digit) {
        case 0:
            //  Set the BCD value on A0-A3. Preserve the values of A4-A7
            PORTC &= B11110000;

            // use a digit value of -1 for blanking
            if (digit_0 > -1) {
                PORTC |= bcd[digit_0];

                // Turn on the digit, The digits are blanked below during the blanking state
                PORTB |= DIGIT_0;
            }
            // move the state to the next digit
            digit += 1;
            break;

        case 1:
            PORTC &= B11110000;
            if (digit_1 > -1) {
                PORTC |= bcd[digit_1];
                PORTB |= DIGIT_1;
            }
            digit += 1;
            break;

        case 2:
            PORTC &= B11110000;
            if (digit_2 > -1) {
                PORTC |= bcd[digit_2];
                PORTB |= DIGIT_2;
            }
            digit += 1;
            break;

        case 3:
            PORTC &= B11110000;
            if (digit_3 > -1) {
                PORTC |= bcd[digit_3];
                PORTB |= DIGIT_3;
            }
            digit += 1;
            break;

        case 4:
            PORTC &= B11110000;
            if (digit_4 > -1) {
                PORTC |= bcd[digit_4];
                PORTB |= DIGIT_4;
            }
            digit += 1;
            break;

        case 5:
            PORTC &= B11110000;
            if (digit_5 > -1) {
                PORTC |= bcd[digit_5];
                PORTB |= DIGIT_5;
            }
            digit = 0;
            break;
        }

        // State is not blanking
        blanking = false;

        // Set the timer to 900uS
        // With the pre-scalar at 64, a count of 0 is 4uS, 1 is 8uS, ...,
        OCR2A = 224; // = [(16*10^6 / 64 ) * 0.000 900] - 1; (must be <256)
    } else {
        // blank_display
        PORTB &= B00000000;

        // State is blanking
        blanking = true;

        // Set the timer to 100uS
        OCR2A = 24; // = [(16*10^6 / 64 ) * 0.000 052] - 1; (must be <256)
    }

#if TIMER_INTERRUPT_DIAGNOSTIC
    PORTD &= ~TIMER_INTERRUPT_TEST_PIN;
#endif
}

void setup() {
    // Enable some minor, chatty, messages on the serial line.
    Serial.begin(BAUD_RATE);
    Serial.println("boot");

    Wire.begin();

    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        Serial.flush();
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

#if 0    
    update_display_with_time();
#endif

    // State machine initial conditions:
    // start up as if the display has cycled once through already
    blanking = true;
    digit = 1;

    // Initialize all I/O pins to output, then set up the inputs/interrupts
    DDRD = B11111111;
    DDRC = B00111111;
    DDRB = B00111111;

    cli(); // stop interrupts

    // This is used for the 1Hz pulse from the clock that triggers
    // time updates.
    pinMode(CLOCK_1HZ, INPUT_PULLUP);

    // time_1Hz_tick() sets a flag that is tested in loop()
    attachInterrupt(digitalPinToInterrupt(CLOCK_1HZ), timer_1HZ_tick_ISR, RISING);

    // MODE_SWITCH is D3
    pinMode(MODE_SWITCH, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(MODE_SWITCH), mode_switch_push, FALLING);

    // INPUT_SWITCH is D4
    pinMode(INPUT_SWITCH, INPUT_PULLUP);
    attachPCINT(digitalPinToPCINT(INPUT_SWITCH), input_switch_push, FALLING);

    // Set up timer 2 - controls the display multiplexing

    // set timer2 interrupt at 950uS. Toggles between 950 and 50 uS
    TCCR2A = 0; // set entire TCCR2A register to 0
    TCCR2B = 0; // same for TCCR0B
    TCNT2 = 0;  // initialize counter value to 0

    // set compare match register for 900uS increments
    OCR2A = 224; // = [(16*10^6 / 64 ) * 0.000 900] - 1; (must be <256)

    // turn on CTC mode
    TCCR2A |= (1 << WGM21);
    // Set CS22 bit for 64 pre-scaler --> B00000100
    TCCR2B |= (1 << CS22);
    // enable timer compare interrupt
    TIMSK2 |= (1 << OCIE2A);

    sei(); // start interrupts
}

void loop() {
    static int tick_count = 0;
    bool get_time = false;

    // Protect 'tick' against update while in use
    cli();
    if (tick) {
        tick = LOW;
        tick_count++;

        if (tick_count >= CLOCK_QUERY_INTERVAL) {
            // update time using I2C access to the clock
            tick_count = 0;
            get_time = true;
        } else {
            TimeSpan ts(1); // a one-second time span
            dt = dt + ts;   // Advance 'dt' by one second

            update_display_using_mode(); // true == adv time by 1s
        }
    }
    sei();

    if (get_time) {
        get_time = false;
        uint32_t start_get_time = micros();
        dt = rtc.now();
        uint32_t get_time_duration = micros() - start_get_time;

        update_display_using_mode();

        if (Serial)
            display_monitor_info(dt, get_time_duration);
    }
}
