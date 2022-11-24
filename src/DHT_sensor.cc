
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

void test_MPL3115A2() {
    float pressure = baro.getPressure();
    float altitude = baro.getAltitude();
    float temperature = baro.getTemperature();

    print(F("------------------------------------\n"));
    print(F("Pressure:    %d hPa\n"), round(pressure));
    print(F("Altitude:    %d m\n"), round(altitude));
    print(F("Temperature: %d °C\n"), round(temperature));
}

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