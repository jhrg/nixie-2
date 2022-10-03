/*
  Four digit clock.

  10/1/22 jhrg
*/

#include <Arduino.h>

#define BAUD_RATE 9600

// This is PORTD (bits 7 to 4)
#define BCD_A PIN4
#define BCD_B PIN5
#define BCD_C PIN6
#define BCD_D PIN7

// PORTC (bits 0 and 1; PORTC has only bits 0 - 5)
#define DIGIT_1 0 // A0
#define DIGIT_2 1 // A1

// BCD codes for the digits 0 to 9
uint8_t bcd[10] = {
    B00000000,
    B00010000,
    B00100000,
    B00110000,
    B01000000,
    B01010000,
    B01100000,
    B01110000,
    B10000000,
    B10010000};

void display_digit(int value, int digit)
{
    uint8_t port_d_low_nyble = PORTD & B00001111;
    PORTD = bcd[value] | port_d_low_nyble;

    PORTC |= B0000001 << digit;
    // digitalWrite(digit, HIGH);
}

void blank_display()
{
    PORTC &= B00000000;
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
        for (int i = 0; i < 100; ++i)
        {
            display_digit(n % 10, DIGIT_1);
            delayMicroseconds(1466);
            blank_display();
            delayMicroseconds(200);

            display_digit(n / 10, DIGIT_2);
            delayMicroseconds(1466);
            blank_display();
            delayMicroseconds(200);

            // simulate time for other four digits
            delayMicroseconds(1666 * 4);
        }

        Serial.print("Display: ");
        Serial.println(n);
    }
}
