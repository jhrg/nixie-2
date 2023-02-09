
#include <Arduino.h>

#include "ambient_light.h"
#include "print.h"

// There are six levels of brightness - with 0 the brightest and
// 5 essentially off.

#define BRIGHTNESS_LEN 6

extern int brightness;

void ambient_light_setup() {
    pinMode(AMBIENT_LIGHT, INPUT);
}

void ambient_light_adjust() {
    uint16_t light = analogRead(AMBIENT_LIGHT);

    // using ceil() means that the value is never < 1, so brightness is
    // always between 0 and BRIGHTNESS_LEN.
    brightness  = BRIGHTNESS_LEN - ceil(BRIGHTNESS_LEN * light/1024.0);

    DPRINTV("light: %d, brightness: %d\n", (int)light, brightness);
}
