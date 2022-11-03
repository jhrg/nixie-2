
#include <Arduino.h>

#include "DHT_sensor.h"

DHT_Unified dht(DHTPIN, DHTTYPE);

// The current display digits
extern volatile int digit_0;
extern volatile int digit_1;
extern volatile int digit_2;
extern volatile int digit_3;
extern volatile int digit_4;
extern volatile int digit_5;

void test_dht_22()
{
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    Serial.println(F("------------------------------------"));
    Serial.println(F("Temperature Sensor"));
    Serial.print(F("Sensor Type: "));
    Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  "));
    Serial.println(sensor.version);
    Serial.print(F("Unique ID:   "));
    Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   "));
    Serial.print(sensor.max_value);
    Serial.println(F("°C"));
    Serial.print(F("Min Value:   "));
    Serial.print(sensor.min_value);
    Serial.println(F("°C"));
    Serial.print(F("Resolution:  "));
    Serial.print(sensor.resolution);
    Serial.println(F("°C"));
    Serial.println(F("------------------------------------"));
    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);
    Serial.println(F("Humidity Sensor"));
    Serial.print(F("Sensor Type: "));
    Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  "));
    Serial.println(sensor.version);
    Serial.print(F("Unique ID:   "));
    Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   "));
    Serial.print(sensor.max_value);
    Serial.println(F("%"));
    Serial.print(F("Min Value:   "));
    Serial.print(sensor.min_value);
    Serial.println(F("%"));
    Serial.print(F("Resolution:  "));
    Serial.print(sensor.resolution);
    Serial.println(F("%"));
    Serial.println(F("------------------------------------"));
#if 0
    DHT_delay_ms = sensor.min_delay / 1000; // not used
#endif
}

// Temperature in F
void update_display_with_weather()
{
    sensors_event_t event;

    dht.temperature().getEvent(&event);
    int temp = 0;
    if (!isnan(event.temperature))
    {
        temp = round(event.temperature * 9.0 / 5.0 + 32.0);
    }

    digit_0 = temp % 10;
    digit_1 = temp / 10;

    int rh = 0;
    dht.humidity().getEvent(&event);
    if (!isnan(event.relative_humidity))
    {
        rh = round(event.relative_humidity);
    }

    digit_2 = rh % 10;
    digit_3 = rh / 10;

    digit_4 = -1;
    digit_5 = -1;
}