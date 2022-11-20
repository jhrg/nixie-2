
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
 * @param fmt A sprintf format string
 * @param ... Variadic arguments matching fmt
 * @return void
*/
void print(const char *fmt, ...) {
    char msg[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);  // copies args
    va_end(ap);

    Serial.print(msg);
}

#if 0
void print(const __FlashStringHelper *fmt, ...) {
    char msg[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), (const char*)fmt, ap);  // copies args
    va_end(ap);

    Serial.print(msg);
}
#endif

void flush() {
    Serial.flush();
}