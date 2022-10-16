/*
  Four digit clock.

  10/1/22 jhrg
*/

#include <Arduino.h>

#include <RTClib.h>

#define BAUD_RATE 9600
#ifndef TIME_OFFSET
#define TIME_OFFSET 10
#endif

// This is PORTC (bits 0 to 3; 5 & 5 are for the I2C bus)
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

#define DIGIT_ON_TIME 950 // uS
#define DIGIT_BLANKING 50 // uS

RTC_DS3231 rtc;

void display_digit(int value, int digit)
{
    // Set the BCD value on A0-A3. Preserve the values of A4-A7
    uint8_t port_d_high_nyble = PORTC & B11110000;
    PORTC = bcd[value] | port_d_high_nyble;

    // PORTB |= B0000001 << digit; // bit(digit)
    switch (digit)
    {
    case 0:
        PORTB |= B0000001;
        break;
    case 1:
        PORTB |= B0000010;
        break;
    case 2:
        PORTB |= B0000100;
        break;
    case 3:
        PORTB |= B0001000;
        break;
    case 4:
        PORTB |= B0010000;
        break;
    case 5:
        PORTB |= B0100000;
        break;
    default:
        Serial.println("FAIL: Unknown digit number");
    }
}

void blank_display()
{
    PORTB &= B00000000;
}

// The state machine
volatile bool blanking;
volatile int digit;

// The current time
volatile int seconds;
volatile int seconds_10;
volatile int minutes;
volatile int minutes_10;
volatile int hours;
volatile int hours_10;

/**
 * @brief Set the global values of time
 */
void get_the_time()
{
    DateTime dt = rtc.now();

    // assume it's likely the seconds have changed
    seconds = dt.second() % 10;
    seconds_10 = dt.second() / 10;

    // minutes and hours change less often
    static int minute = -1;
    if (minute != dt.minute())
    {
        minute = dt.minute();
        minutes = dt.minute() % 10;
        minutes_10 = dt.minute() / 10;
    }

    static int hour = -1;
    if (hour != dt.hour())
    {
        hour = dt.hour();
        hours = dt.hour() % 10;
        hours_10 = dt.hour() / 10;
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

volatile byte tick = LOW;

/**
 * @brief Record that one second has elapsed
 */
void timer_1HZ_tick_ISR()
{
    tick = HIGH;
}

void setup()
{
    // Enable some minor, chatty, messages on the serial line.
    Serial.begin(BAUD_RATE);
    Serial.println("boot");

    Wire.begin();

    if (!rtc.begin())
    {
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

    if (abs(now.unixtime() - build_time.unixtime()) > 60)
    {
        Serial.print("Adjusting the time: ");
        print_time(build_time, true);

        rtc.adjust(build_time);
    }
#endif

    rtc.disable32K();
    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);

    Serial.print("time: ");
    print_time(rtc.now(), true);

    get_the_time();

    // State machine initial conditions:
    // start up as if the display has cycled once through already
    blanking = true;
    digit = 1;

    // delay(10000); // Not sure we need this...

    // Initialize all I/O pins to output, then one for the interrupt
    DDRD = B11111111;
    DDRC = B00111111;
    DDRB = B00111111;

    pinMode(2, INPUT_PULLUP);

    // time_1Hz_tick() sets a flag that is tested in loop()
    attachInterrupt(digitalPinToInterrupt(2), timer_1HZ_tick_ISR, RISING);

    // Set up timer 2 - controls the display multiplexing
    cli(); // stop interrupts

    // set timer2 interrupt at 950uS. Toggles between 950 and 50 uS
    TCCR2A = 0; // set entire TCCR2A register to 0
    TCCR2B = 0; // same for TCCR0B
    TCNT2 = 0;  // initialize counter value to 0

    // set compare match register for 950uS increments
    OCR2A = 236; // = [(16*10^6 / 64 ) * 0.00095] - 1; (must be <256)
    // use OCR0A of 99, with a pre-scaler of 8 for 50uS
    // turn on CTC mode
    TCCR2A |= (1 << WGM21);
    // Set CS01 and CS00 bits for 64 prescaler
    TCCR2B |= (1 << CS21) | (1 << CS20);
    // enable timer compare interrupt
    TIMSK2 |= (1 << OCIE2A);

    sei(); // start interrupts
}

/**
 * @brief The display multiplexing code
 */
ISR(TIMER2_COMPA_vect)
{
    cli(); // stop interrupts

    if (blanking)
    {
        switch (digit)
        {
        case 0:
            display_digit(seconds, 0);
            digit += 1;
            break;
        case 1:
            display_digit(seconds_10, 1);
            digit += 1;
            break;
        case 2:
            display_digit(seconds, 2);
            digit += 1;
            break;
        case 3:
            display_digit(seconds, 3);
            digit += 1;
            break;
        case 4:
            display_digit(seconds, 4);
            digit += 1;
            break;
        case 5:
            display_digit(seconds, 5);
            digit = 0;
            break;
        }

        // State is not blanking
        blanking = false;

        // Set the timer to 950uS
        OCR2A = 236; // = [(16*10^6 / 64 ) * 0.000 950] - 1; (must be <256)
        TCCR2B |= (1 << CS21) | (1 << CS20);
    }
    else
    {
        blank_display();

        // State is blanking
        blanking = true;

        // Set the timer to 50uS
        OCR2A = 99; // = [(16*10^6 / 8 ) * 0.000 050] - 1; (must be <256)
        TCCR2B |= (1 << CS21);
    }

    sei(); // start interrupts
}

void display_monitor_info()
{
    static unsigned int n = 0;
    Serial.print("Display: ");
    Serial.print(n++);
    Serial.print(", ");

    print_time(rtc.now(), true);
}

void loop()
{
    int tick_count = 0;
    if (tick)
    {
        tick = LOW;
        tick_count++;

        if (tick_count < 5)
        {
            // update
            tick_count = 0;
        }

        get_the_time();

        if (Serial)
        {
            display_monitor_info();
        }
    }
}
