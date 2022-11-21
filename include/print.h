
/**
 * A version of printf for the Arduino.
 * 11/20/22
 * James Gallagher <jhrg@mac.com>
 */

void print(const char *fmt, ...);
void print(const __FlashStringHelper *fmt, ...);

void flush();

// Using F() in this macro reduced RAM use from 67% to 50% in ~1200 LOC
#if DEBUG
#define DPRINTV(fmt, ...) print(F(fmt), __VA_ARGS__)
#define DPRINT(fmt) print(F(fmt))
#else
#define DPRINTV(fmt, ...)
#define DPRINT(fmt)
#endif
