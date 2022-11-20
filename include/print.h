
/**
 * A version of printf for the Arduino.
 * 11/20/22
 * James Gallagher <jhrg@mac.com>
 */

void print(const char *fmt, ...);
void flush();

#if 0
// Using F() in this macro reduced RAM use from 67% to 50% in ~1200 LOC
#if DEBUG
#define DPRINTV(fmt, ...) print((const char *)F(fmt), __VA_ARGS__)
#define DPRINT(fmt) print((const char *)F(fmt))
#else
#define DPRINTV(fmt, ...)
#define DPRINT(fmt)
#endif
#endif