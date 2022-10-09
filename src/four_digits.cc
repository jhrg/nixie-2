/*
  Four digit clock.

  10/1/22 jhrg
*/

#include <Arduino.h>

#define BAUD_RATE 9600

// This is PORTC (bits 0 to 3; 5 & 5 are for the I2C bus)
#define BCD_A A0
#define BCD_B A1
#define BCD_C A2
#define BCD_D A3

// PORTB 8 - 13 (bits 0 - 5)
#define SECONDS 0
#define SECONDS_10 1

#define MINUTES 2
#define MINUTES_10 3

#define HOURS 4
#define HOURS_10 15

#define DIGIT_ON_TIME 950   // uS
#define DIGIT_BLANKING 50   // uS

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

void display_digit(int value, int digit)
{
    uint8_t port_d_high_nyble = PORTC & B11110000;
    PORTC = bcd[value] | port_d_high_nyble;

    PORTB |= B0000001 << digit; // bit(digit)
    // digitalWrite(digit, HIGH);
}

void blank_display()
{
    PORTB &= B00000000;
    // digitalWrite(digit, LOW);
}

void setup()
{
    // Enable some minor, chatty, messages on the serial line.
    Serial.begin(BAUD_RATE);
    Serial.println("boot");

    // Initialize all I/O pins to output
    DDRD = B11111111;
    DDRC = B00111111;
    DDRB = B00111111;
}

void loop()
{
    for (int n = 0; n < 100; ++n)
    {
        for (int i = 0; i < 167; ++i)
        {
            display_digit(n % 10, SECONDS);
            delayMicroseconds(DIGIT_ON_TIME);
            blank_display();
            delayMicroseconds(DIGIT_BLANKING);

            display_digit(n / 10, SECONDS_10);
            delayMicroseconds(DIGIT_ON_TIME);
            blank_display();
            delayMicroseconds(DIGIT_BLANKING);

            // simulate time for other four digits
            delayMicroseconds((DIGIT_ON_TIME + DIGIT_BLANKING) * 4);
        }

        Serial.print("Display: ");
        Serial.println(n);
    }
}
