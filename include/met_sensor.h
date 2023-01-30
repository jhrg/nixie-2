
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

#define inch_Hg_per_hPa 0.02953
#define SEALEVELPRESSURE_HPA (1013.25)

#define WEATHER_DISPLAY_DURATION 12  // seconds

void update_display_with_weather();
bool init_bme280();
void test_bme280();
