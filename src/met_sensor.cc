
#include <Arduino.h>

#include "met_sensor.h"
#include "print.h"

Adafruit_BME280 bme;  // I2C

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

bool init_bme280() {
    // default settings
    bool status = bme.begin(0x76);
    // You can also pass in a Wire library object like &Wire2
    // status = bme.begin(0x76, &Wire2)
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x");
        Serial.println(bme.sensorID(), 16);
    }

    return status;
}

void test_bme280() {
    float pressure = bme.readPressure() / 100.0F;
    float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();

    print(F("------------------------------------\n"));
    print(F("Pressure:    %d.%02d hPa\n"), round(pressure), frac(pressure));
    print(F("Altitude:    %d.%02d m\n"), round(altitude), frac(altitude));

    print(F("Temperature: %d.%02d °C\n"), round(temperature), frac(temperature));
    print(F("Relative humidity: %d.%02d °C\n\n"), round(humidity), frac(humidity));
}

// for altitude correction: 1 hPa decrease per 30 feet above MS
static float station_msl = 5560.0;  // feet; could be a config param
static float hPa_station_correction = station_msl / 30.0;
;

// The weather display is a simple state machine:
// 0,..., N/2 - 1: show temperature, humidity
// N/2, ..., N-1: show pressure
void update_display_with_weather() {
    static int state = 0;
    if (state > WEATHER_DISPLAY_DURATION - 1)
        state = 0;

    DPRINTV("Weather state: %d\n", state);

    switch (state) {
        case 0: {
            float temperature = bme.readTemperature() * 9.0 / 5.0 + 32.0;

            int LHS = (int)temperature;
            int RHS = (int)((temperature - LHS) * 100.0);

            DPRINTF("Temperature: ", temperature);
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

#if DEBUG
            print_digits(true);
#endif
            break;
        }

        case 4: {
            float humidity = bme.readHumidity();

            int LHS = (int)humidity;
            int RHS = (int)((humidity - LHS) * 100.0);

            DPRINTV("LHS: %d\n", LHS);
            DPRINTV("RHS: %d\n", RHS);

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

        case 8: {
            float pressure = (bme.readPressure() / 100.0F + hPa_station_correction) * inch_Hg_per_hPa;

            int LHS = (int)pressure;
            int RHS = (int)((pressure - LHS) * 100.0);

            DPRINTV("LHS: %d\n", LHS);
            DPRINTV("RHS: %d\n", RHS);

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