
#include <Arduino.h>

#include "DHT_sensor.h"

DHT_Unified dht(DHTPIN, DHTTYPE);
Adafruit_MPL3115A2 baro;

extern void print(const char *fmt, ...);
extern void print_digits(bool newline);

// The current display digits
extern volatile int digit_0;
extern volatile int digit_1;
extern volatile int digit_2;
extern volatile int digit_3;
extern volatile int digit_4;
extern volatile int digit_5;

extern volatile int d0_rhdp;
extern volatile int d1_rhdp;
extern volatile int d2_rhdp;
extern volatile int d3_rhdp;
extern volatile int d4_rhdp;
extern volatile int d5_rhdp;

extern volatile int d0_lhdp;
extern volatile int d1_lhdp;
extern volatile int d2_lhdp;
extern volatile int d3_lhdp;
extern volatile int d4_lhdp;
extern volatile int d5_lhdp;

extern void blank_dp();     // defined in four_digits.cc

void test_dht_22() {
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
}

void test_MPL3115A2() {
    float pressure = baro.getPressure();
    float altitude = baro.getAltitude();
    float temperature = baro.getTemperature();

    Serial.println(F("------------------------------------"));
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
void update_display_with_weather(int state) {
    sensors_event_t event;

    Serial.print("Weather state: ");
    Serial.println(state);

    switch (state) {
    case 0: {
        int trials = -1;
        do
        {
            dht.temperature().getEvent(&event);
            delay(10);
            trials++;
        } while (isnan(event.temperature) && trials < 10);

        int temp = 0;
        if (!isnan(event.temperature)) {
            temp = round(event.temperature * 9.0 / 5.0 + 32.0);
        }
        int rh = 0;
        // dht.humidity().getEvent(&event);
        if (!isnan(event.relative_humidity)) {
            rh = round(event.relative_humidity);
        }

        digit_0 = temp % 10;
        digit_1 = temp / 10;
        digit_2 = -1;
        digit_3 = -1;
        digit_4 = rh % 10;
        digit_5 = rh / 10;
#if DEBUG
        print("Trials: %d\n", trials);
        print_digits(true);
#endif
        break;
    }

    case WEATHER_DISPLAY_DURATION:
    {
        blank_dp();
        
        // for altitude correction: 1 hPa decrease per 30 feet above MSL
        float station_msl = 5560.0; // feet
        float hPa_station_correction = station_msl / 30.0; // pre compute
        float pressure = (baro.getPressure() + hPa_station_correction) * inch_Hg_per_hPa;

        int LHS = (int)pressure;
        int RHS = (int)((pressure - LHS) * 100.0);
#if DEBUG
        Serial.print("LHS: "); Serial.println(LHS);
        Serial.print("RHS: "); Serial.println(RHS);
#endif
        digit_0 = RHS % 10;
        digit_1 = RHS / 10;
        d2_rhdp = 1;
        digit_2 = LHS % 10;
        digit_3 = LHS / 10;
        digit_4 = -1;
        digit_5 = -1;
#if DEBUG
        print_digits(true);
#endif
        break;
    }

    default:
        break;
    }
}