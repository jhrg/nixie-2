
/**
 * A version of printf for the Arduino.
 * 11/20/22
 * James Gallagher <jhrg@mac.com>
 */

#include <Arduino.h>

/**
 * @brief A printf() clone
 *
 * Use sprintf to format data for printing and pass the resulting string to
 * Arduino Serial.print(). Does not print a newline, so include that in 'fmt.'
 * 
 * @note The printf family on the Atmel 328 processors does not support float
 * or double values without rebuilding one of the libraries. Use %d and round()
 * or break the float into two parts.
 *
 * @param fmt A sprintf format string
 * @param ... Variadic arguments matching fmt
 * @return void
 */
void print(const char *fmt, ...) {
    char msg[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap); // copies args
    va_end(ap);

    Serial.print(msg);
}

void print(const __FlashStringHelper *flash_fmt, ...) {
    // Copy the string from flash to RAM for use with vsnprintf()
    char fmt[128];
    strncpy_P(fmt, (const char *)flash_fmt, sizeof(fmt));

    va_list ap;
    va_start(ap, flash_fmt);
    char msg[128];
    vsnprintf(msg, sizeof(msg), fmt, ap); // copies args
    va_end(ap);

    Serial.print(msg);
}

#if 0
// another potential solution (that might work)
// See https://forum.arduino.cc/t/function-that-handles-both-char-and-__flashstringhelper-strings/512100/11

char getCharFromRam(const void *);
char getCharFromFlash(const void *);
void test(const char *);
void test(const __FlashStringHelper *);
void doit(char (*)(const void *), const void *);

void setup() {
	Serial.begin(115200);
	delay(1000);

	Serial.println(F("----------------"));
	test("This is a normal string.");
	Serial.println();

	Serial.println(F("----------------"));
	test(F("This is a flash string."));
	Serial.println();
}

void loop() {
}

void test(const char *line) {
	doit(getCharFromRam, (const void*) line);
}

void test(const __FlashStringHelper *line) {
	doit(getCharFromFlash, (const void*) line);
}

char getNextByte(unsigned int *address, byte stringType)
{
  char ch;

  if (address == NULL) return '\0';

  if (stringType == 0) // RAM
  {
    char *ptr = (char*)*address;
    ch = *ptr;
  }
  else // FLASH
  {
    ch = pgm_read_byte_near(*address);
  }
  
  (*address)++;

  return ch;
}

void doit(char (*funct)(const void *), const void *string) {
	char ch;
	const char *ptr = (const char *) string;

	while ((ch = funct(ptr++))) {
		Serial.print(ch);
	}
}

char getCharFromRam(const void *ptr) {
	return *((char *) ptr);
}

char getCharFromFlash(const void *ptr) {
	return pgm_read_byte_near((char * )ptr);
}

// OR

void test(const char *);
void test(const __FlashStringHelper *);
void doit(char (*)(const char *), const char *);

void setup() {
	Serial.begin(115200);
	delay(1000);

	Serial.println(F("----------------"));
	test("This is a normal string.");
	Serial.println();

	Serial.println(F("----------------"));
	test(F("This is a flash string."));
	Serial.println();
}

void loop() {
}

void test(const char *line) {
	doit([](const char *ptr) {return *ptr;}, line);
}

void test(const __FlashStringHelper *line) {
	doit([](const char *ptr) {return (char) pgm_read_byte_near(ptr);},
			(const char*) line);
}

void doit(char (*funct)(const char *), const char *string) {
	char ch;

	while ((ch = funct(string++))) {
		Serial.print(ch);
	}
}
#endif

void flush() {
    Serial.flush();
}