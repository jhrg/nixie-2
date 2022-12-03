
#include <Adafruit_Sensor.h>

#if USE_MPL3115A2
#include <Adafruit_MPL3115A2.h>
#else
#include <Adafruit_BME280.h>
#endif

#define inch_Hg_per_hPa 0.02953
#define SEALEVELPRESSURE_HPA (1013.25)

#if USE_MPL3115A2
bool init_mpl3115a2();
void test_MPL3115A2();
#define WEATHER_DISPLAY_DURATION 8  // seconds
#else
bool init_bme280();
void test_bme280();
#define WEATHER_DISPLAY_DURATION 12  // seconds
#endif

void update_display_with_weather();
