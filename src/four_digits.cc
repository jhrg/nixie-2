/*
  Four digit clock.

  10/1/22 jhrg
*/

#include <Arduino.h>

#define BAUD_RATE 9600

#define BCD_A PIN4
#define BCD_B PIN5
#define BCD_C PIN6
#define BCD_D PIN7

struct bcd_code
{
    int d;
    int c;
    int b;
    int a;
} codes[10];

void display_digit(struct bcd_code code)
{
    digitalWrite(BCD_A, code.a);
    digitalWrite(BCD_B, code.b);
    digitalWrite(BCD_C, code.c);
    digitalWrite(BCD_D, code.d);
}

void setup()
{
    // Enable some minor, chatty, messages on the serial line.
    Serial.begin(BAUD_RATE);
    Serial.println("boot");

    pinMode(BCD_A, OUTPUT);
    pinMode(BCD_B, OUTPUT);
    pinMode(BCD_C, OUTPUT);
    pinMode(BCD_D, OUTPUT);

    // D C B A
    codes[0] = {LOW, LOW, LOW, LOW};
    codes[1] = {LOW, LOW, LOW, HIGH};
    codes[2] = {LOW, LOW, HIGH, LOW};
    codes[3] = {LOW, LOW, HIGH, HIGH};
    codes[4] = {LOW, HIGH, LOW, LOW};
    codes[5] = {LOW, HIGH, LOW, HIGH};
    codes[6] = {LOW, HIGH, HIGH, LOW};
    codes[7] = {LOW, HIGH, HIGH, HIGH};
    codes[8] = {HIGH, LOW, LOW, LOW};
    codes[9] = {HIGH, LOW, LOW, HIGH};
}

void loop()
{
    for (int i = 0; i < 10; ++i)
    {
        display_digit(codes[i]);
        Serial.print("Display: ");
        Serial.println(i);
        delay(1000);
    }
}
