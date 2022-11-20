
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

extern void blank_dp();     // defined in four_digits.cc

void test_dht_22() {
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);

    // Temperature sensor details
    DPRINT("------------------------------------");
    DPRINT("Temperature Senor");
    DPRINTV("Sensor Type: %s\n", sensor.name);
    DPRINTV("Driver Ver:  %d\n", sensor.version);
    DPRINTV("Unique ID:   %d\n", sensor.sensor_id);
    DPRINTV("Max Value:   %f °C\n", sensor.max_value);
    DPRINTV("Min Value:   %f °C\n", sensor.min_value);
    DPRINTV("Resolution:  %f °C\n", sensor.resolution);
   
    dht.humidity().getSensor(&sensor);

    // Print humidity sensor details.
    DPRINT("------------------------------------");
    DPRINT("Humidity Sensor");
    DPRINTV("Sensor Type: %s\n", sensor.name);
    DPRINTV("Driver Ver:  %d\n", sensor.version);
    DPRINTV("Unique ID:   %d\n", sensor.sensor_id);
    DPRINTV("Max Value:   %f %\n", sensor.max_value);
    DPRINTV("Min Value:   %f %\n", sensor.min_value);
    DPRINTV("Resolution:  %f %\n", sensor.resolution);

}

void test_MPL3115A2() {
    float pressure = baro.getPressure();
    float altitude = baro.getAltitude();
    float temperature = baro.getTemperature();

    DPRINT("------------------------------------");
    DPRINTV("pressure:    %f hPa\n", pressure);
    DPRINTV("altitude:    %f m\n", altitude);
    DPRINTV("temperature: %f °C\n", temperature);
}

// These hold the most recent measurement. Sometimes the DHT
// does not return a value, so these provide continuity. They
// are loaded with values during boot time.
static float temperature = 0.0;
static float relative_humidity = 0.0;

// for altitude correction: 1 hPa decrease per 30 feet above MS
static float station_msl = 5560.0;  // feet; could be a config param
static float hPa_station_correction = 0.0;

/**
 * @brief initialize the DHT22 sensor
 * @return False if either of the temperature or relative humidity readings fails
 */
bool initialize_DHT_values() {
    sensors_event_t event;
    int trial = 0;
    bool status = true;

    dht.temperature().getEvent(&event);
    while (isnan(event.temperature) && trial < 10) {
        delay(2000);  // 2s
        trial++;
        dht.temperature().getEvent(&event);
    }
    if (!isnan(event.temperature)) {
        temperature = event.temperature;
    } else {
        print("DHT Temperature failure on boot\n");
        status = false;
    }

    trial = 0;
    dht.humidity().getEvent(&event);
    while (isnan(event.relative_humidity) && trial < 10) {
        delay(2000);  // 2s
        trial++;
        dht.temperature().getEvent(&event);
    }
    if (!isnan(event.relative_humidity)) {
        relative_humidity = event.relative_humidity;
    } else {
        print("DHT relative humidity failure on boot\n");
        status = false;
    }

    hPa_station_correction = station_msl / 30.0;

    return status;
}

// The weather display is a simple state machine:
// 0,..., N/2 - 1: show temperature, humidity
// N/2, ..., N-1: show pressure
void update_display_with_weather() {
    static int state = 0;
    if (state > WEATHER_DISPLAY_DURATION - 1) state = 0;

    DPRINTV("Weather state: %d\n", state);

    switch (state) {
        case 0: {
            blank_dp();

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
#if DEBUG
            print_digits(true);
#endif
            break;
        }

        case 4: {
            blank_dp();
            float pressure = (baro.getPressure() + hPa_station_correction) * inch_Hg_per_hPa;

            int LHS = (int)pressure;
            int RHS = (int)((pressure - LHS) * 100.0);

            DPRINTV("LHS: %d\n", LHS);
            DPRINTV("RHS: %d\n", RHS);

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