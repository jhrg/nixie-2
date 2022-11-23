
#include <Arduino.h>

#include "DHT_sensor.h"
#include "print.h"

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

extern void blank_dp(); // defined in four_digits.cc

#if USE_DHT
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
#endif

void test_MPL3115A2() {
    float pressure = baro.getPressure();
    float altitude = baro.getAltitude();
    float temperature = baro.getTemperature();

    print(F("------------------------------------\n"));
    print(F("Pressure:    %d hPa\n"), round(pressure));
    print(F("Altitude:    %d m\n"), round(altitude));
    print(F("Temperature: %d °C\n"), round(temperature));
}

#if USE_DHT
// These hold the most recent measurement. Sometimes the DHT
// does not return a value, so these provide continuity. They
// are loaded with values during boot time.
static float temperature = 0.0;
static float relative_humidity = 0.0;

void initialize_DHT_values() {
    sensors_event_t event;
    int trial = 0;

    dht.temperature().getEvent(&event);
    while (isnan(event.temperature) && trial < 10) {
        delay(2000); // 2s
        trial++;
        dht.temperature().getEvent(&event);
    }
    if (!isnan(event.temperature)) {
        temperature = event.temperature;
    } else {
        print("DHT Temperature failure on boot\n");
    }

    trial = 0;
    dht.humidity().getEvent(&event);
    while (isnan(event.relative_humidity) && trial < 10) {
        delay(2000); // 2s
        trial++;
        dht.temperature().getEvent(&event);
    }
    if (!isnan(event.relative_humidity)) {
        relative_humidity = event.relative_humidity;
    } else {
        print("DHT relative humidity failure on boot\n");
    }

    hPa_station_correction = station_msl / 30.0;
}
#endif

// for altitude correction: 1 hPa decrease per 30 feet above MS
static float station_msl = 5560.0; // feet; could be a config param
static float hPa_station_correction = station_msl / 30.0;
;

// The weather display is a simple state machine:
// 0,..., N/2 - 1: show temperature, humidity
// N/2, ..., N-1: show pressure
void update_display_with_weather() {
    // TODO Remove static float hPa_station_correction = station_msl / 30.0;
    static int state = 0;
    if (state > WEATHER_DISPLAY_DURATION - 1)
        state = 0;

    DPRINTV("Weather state: %d\n", state);

    switch (state) {
    case 0: {
        float temperature = baro.getTemperature() * 9.0 / 5.0 + 32.0;

        int LHS = (int)temperature;
        int RHS = (int)((temperature - LHS) * 100.0);

        DPRINTV("LHS: %d\n", LHS);
        DPRINTV("RHS: %d\n", RHS);

        blank_dp();
        digit_0 = RHS / 10;
        d1_rhdp = 1;
        digit_1 = LHS % 10;
        digit_2 = LHS / 10;

        digit_3 = -1;
        digit_4 = -1;
        digit_5 = -1;
#if USE_DHT
        sensors_event_t event;
        dht.temperature().getEvent(&event);
        if (!isnan(event.temperature)) {
            temperature = event.temperature;
        } else {
            print("Failure to read temperature - DHT\n");
        }
        int temp = round(temperature * 9.0 / 5.0 + 32.0);

        dht.humidity().getEvent(&event);
        if (!isnan(event.relative_humidity)) {
            relative_humidity = event.relative_humidity;
        } else {
            print("Failure to read humidity - DHT\n");
        }
        int rh = round(relative_humidity);

        digit_0 = temp % 10;
        digit_1 = temp / 10;
        d0_rhdp = 1;

        digit_2 = -1;
        digit_3 = -1;

        digit_4 = rh % 10;
        digit_5 = rh / 10;
        d4_rhdp = 1;
#endif

#if DEBUG
        print_digits(true);
#endif
        break;
    }

    case 4: {
        float pressure = (baro.getPressure() + hPa_station_correction) * inch_Hg_per_hPa;

        int LHS = (int)pressure;
        int RHS = (int)((pressure - LHS) * 100.0);

        DPRINTV("LHS: %d\n", LHS);
        DPRINTV("RHS: %d\n", RHS);
#if 0
            Serial.print("LHS: ");
            Serial.println(LHS);
            Serial.print("RHS: ");
            Serial.println(RHS);
#endif
        blank_dp();
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

    state += 1;
}