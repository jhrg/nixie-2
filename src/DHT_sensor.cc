
#include <Arduino.h>

#include "DHT_sensor.h"

DHT_Unified dht(DHTPIN, DHTTYPE);
Adafruit_MPL3115A2 baro;

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
}

void test_MPL3115A2()
{
    float pressure = baro.getPressure();
    float altitude = baro.getAltitude();
    float temperature = baro.getTemperature();

    Serial.println("-----------------");
    Serial.print("pressure = ");
    Serial.print(pressure);
    Serial.println(" hPa");
    Serial.print("altitude = ");
    Serial.print(altitude);
    Serial.println(" m");
    Serial.print("temperature = ");
    Serial.print(temperature);
    Serial.println(" C");
}

// The weather display is a simple state machine:
// 1,2: show temperature, humidity
// 3,4: show pressure
void update_display_with_weather(int state)
{
    sensors_event_t event;

    switch (state)
    {
    case 1:
    case 2:
    {
        dht.temperature().getEvent(&event);
        int temp = 0;
        if (!isnan(event.temperature))
        {
            temp = round(event.temperature * 9.0 / 5.0 + 32.0);
        }
        int rh = 0;
        // dht.humidity().getEvent(&event);
        if (!isnan(event.relative_humidity))
        {
            rh = round(event.relative_humidity);
        }

        digit_0 = temp % 10;
        digit_1 = temp / 10;
        digit_2 = -1;
        digit_3 = -1;
        digit_4 = rh % 10;
        digit_5 = rh / 10;
        break;
    }

    case 3:
    case 4:
    {
        float pressure = baro.getPressure();
#if 0
        float altitude = baro.getAltitude();
        float temperature = baro.getTemperature();
#endif
        int LHS = (int)pressure;
        int RHS = (int)((pressure - LHS) * 100);
        digit_0 = RHS % 10;
        digit_1 = RHS / 10;
        digit_2 = LHS % 10;
        digit_3 = LHS / 10;
        digit_4 = -1;
        digit_5 = -1;
        break;
    }

    default:
        break;
    }
}