
#include <DHT_U.h>
#include <Adafruit_MPL3115A2.h>


#define DHTPIN 5      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)

void test_dht_22();
void test_MPL3115A2();

void update_display_with_weather(int state);

