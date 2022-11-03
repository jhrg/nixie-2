
#include <Arduino.h>

#include "DHT_sensor.h"

#if 0
DHT_Unified dht(DHTPIN, DHTTYPE);
#endif
DHT_nonblocking dht(DHTPIN, DHTTYPE);

// The current display digits
extern volatile int digit_0;
extern volatile int digit_1;
extern volatile int digit_2;
extern volatile int digit_3;
extern volatile int digit_4;
extern volatile int digit_5;

void setup_dht()
{
    // Temperature and humidity sensor
    // dht.begin();

    test_dht_22();
}
void test_dht_22()
{
#if 0
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
#endif
#if 0
    DHT_delay_ms = sensor.min_delay / 1000; // not used
#endif
}

/*
 * Poll for a measurement, keeping the state machine alive.  Returns
 * true if a measurement is available.
 */
static void measure_environment(float *temperature, float *humidity)
{
    static unsigned long measurement_timestamp = millis();
    static float temp = 0.0;
    static float rh = 0.0;

    /* Measure once every four seconds. */
    if (millis() - measurement_timestamp > 4000ul)
    {
        if (dht.measure(temperature, humidity) == true)
        {
            measurement_timestamp = millis();
            temp = *temperature;
            rh = *humidity;
        }
    }
    else
    {
        *temperature = temp;
        *humidity = rh;
    }
}

// Temperature in F
void update_display_with_weather()
{
    float temperature;
    float humidity;
    measure_environment(&temperature, &humidity);

    int temp = round(temperature * 9.0 / 5.0 + 32.0);
    digit_0 = temp % 10;
    digit_1 = temp / 10;

    int rh = round(humidity);
    digit_2 = rh % 10;
    digit_3 = rh / 10;

    digit_4 = -1;
    digit_5 = -1;
}