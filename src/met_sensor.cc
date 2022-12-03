
#include <Arduino.h>

#include "met_sensor.h"
#include "print.h"

#if 0
Adafruit_MPL3115A2 baro;
#endif

Adafruit_BME280 bme;  // I2C

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
    bool status = bme.begin();
    // You can also pass in a Wire library object like &Wire2
    // status = bme.begin(0x76, &Wire2)
    if (!status) {
        DPRINT("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        DPRINTV("SensorID was: 0x%x\n", bme.sensorID());
    }

    return status;
}

void test_bme280() {
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" °C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
}

#if 0
void test_MPL3115A2() {
    float pressure = baro.getPressure();
    float altitude = baro.getAltitude();
    float temperature = baro.getTemperature();

    print(F("------------------------------------\n"));
    print(F("Pressure:    %d.%02d hPa\n"), round(pressure), frac(pressure));
    print(F("Altitude:    %d.%02d m\n"), round(altitude), frac(altitude));
    print(F("Temperature: %d.%02d °C\n"), round(temperature), frac(temperature));
}
#endif

// for altitude correction: 1 hPa decrease per 30 feet above MS
static float station_msl = 5560.0; // feet; could be a config param
static float hPa_station_correction = station_msl / 30.0;

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
        float temperature = bme.readTemperature();
        temperature = temperature * 9.0 / 5.0 + 32.0;

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
        float pressure = bme.readPressure() / 100.0F;
        pressure = (pressure + hPa_station_correction) * inch_Hg_per_hPa;

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

    case 8: {
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

    default:
        break;
    }

    state += 1;
}