
#include <DHT_U.h>
#include <Adafruit_MPL3115A2.h>


#define DHTPIN 5      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)

#define inch_Hg_per_hPa 0.02953
#define WEATHER_DISPLAY_DURATION 8  // seconds

void test_dht_22();
void test_MPL3115A2();

void update_display_with_weather();
void initialize_DHT_values();
