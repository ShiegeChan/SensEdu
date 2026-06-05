/*
 * Weather math, classification & report printers.
 *
 * Provides sea-level pressure conversion, dew-point calculation, coarse
 * classifiers for each measured quantity, and the Serial print helpers used
 * by the main loop.
 */

#include <math.h>
#include "pressure_types.h"

/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

#define LABEL_WIDTH    16
#define VALUE_WIDTH    14

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static const char* classify_humidity(float humidity);
static const char* classify_dew_spread(float spread);
static const char* classify_pressure(float pressure_hpa);
static const char* classify_pressure_trend(float trend_hpa_per_hour);

static uint32_t utf8_visible_len(const char* s);
static void print_padded(const char* text, uint32_t width);
static void print_row_header(const char* label);
static void print_row_status(const char* status);
static void print_value_padded(float value, uint8_t decimals, const char* unit);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

// Converts a pressure measurement at the given altitude to its equivalent sea-level pressure.
//
// Standard pressure at sea level is ~1013.25 hPa. Pressure decreases with altitude.
// A station-level reading must be normalised before it can be compared against typical weather thresholds.
float calculate_sea_lvl_pressure_hpa(float pressure_pa, float temp, float altitude) {
    float temp_k = temp + 273.15f;
    float sea_lvl_pa = pressure_pa * pow((temp_k + 0.0065f * altitude) / temp_k, 5.257f);
    return sea_lvl_pa / 100.0f;
}

// Calculates dew point based on Magnus formula.
//
// Pass raw sensor readings. This calculation expects measurements that describe 
// the same physical state of the air.
//
// Suggested to use SHT temperature here, not the DPS one since
// the SHT metrics come from the same die at the same instant.
float calculate_dew_point(float temp, float humidity) {
    const float a = 17.625f;
    const float b = 243.04f;
    float gamma = (a * temp) / (b + temp) + log(humidity / 100.0f);
    return (b * gamma) / (a - gamma);
}

// Recovers ambient RH from a measured dew point at a given ambient temperature.
// Basically an inverse of calculate_dew_point().
float calculate_relative_humidity(float temp, float dew_point) {
    const float a = 17.625f;
    const float b = 243.04f;
    float gamma_dp = (a * dew_point) / (b + dew_point);
    float gamma_t = (a * temp) / (b + temp);
    float rh = 100.0f * exp(gamma_dp - gamma_t);
    if (rh > 100.0f) rh = 100.0f;
    if (rh < 0.0f)   rh = 0.0f;
    return rh;
}

void report_temp(float temp) {
    print_row_header("Temperature");
    print_value_padded(temp, 2, "°C");
    print_row_status("");
}

void report_humidity(float rh) {
    print_row_header("Humidity");
    print_value_padded(rh, 2, "%");
    print_row_status(classify_humidity(rh));
}

void report_dew_point(float dp) {
    print_row_header("Dew Point");
    print_value_padded(dp, 2, "°C");
    print_row_status(classify_dew_point(dp));
}

void report_dew_spread(float spread) {
    print_row_header("Dew Spread");
    print_value_padded(spread, 2, "°C");
    print_row_status(classify_dew_spread(spread));
}

void report_altitude(float altitude) {
    print_row_header("Altitude");
    print_value_padded(altitude, 2, "m");
    print_row_status("");
}

void report_pressure(float pressure_hpa) {
    print_row_header("Pressure (raw)");
    print_value_padded(pressure_hpa, 2, "hPa");
    print_row_status("");
}

void report_sea_level_pressure(float pressure_hpa) {
    print_row_header("Pressure (sea)");
    print_value_padded(pressure_hpa, 2, "hPa");
    print_row_status(classify_pressure(pressure_hpa));
}

void report_pressure_trend(void) {
    PressureTrendStatus status;
    get_pressure_trend_status(&status);

    char value[24];
    char third_col[64];
    uint32_t remaining_min = status.ms_until_next_capture / 60000UL;
    uint32_t hh = remaining_min / 60;
    uint32_t mm = remaining_min % 60;

    print_row_header("Pressure Trend");

    if (!status.trend_available) {
        snprintf(value, sizeof(value), "%u/%u samples", status.samples_captured, status.samples_required);
        print_padded(value, VALUE_WIDTH);
        snprintf(third_col, sizeof(third_col), "Collecting (next in %u:%02u)", (unsigned)hh, (unsigned)mm);
    } else {
        snprintf(value, sizeof(value), "%.2f hPa/h", status.trend_hpa_per_hour);
        print_padded(value, VALUE_WIDTH);
        snprintf(third_col, sizeof(third_col), "%s (next in %u:%02u)", classify_pressure_trend(status.trend_hpa_per_hour), (unsigned)hh, (unsigned)mm);
    }

    print_row_status(third_col);
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */

static const char* classify_humidity(float humidity) {
    if (humidity < 30.0f) return "Dry";
    if (humidity < 60.0f) return "Comfortable";
    if (humidity < 75.0f) return "Humid";
    if (humidity < 90.0f) return "Very Humid";
    return "Saturated";
}

static const char* classify_dew_point(float dp) {
    if (dp <  5.0f) return "Dry";
    if (dp < 15.0f) return "Comfortable";
    if (dp < 20.0f) return "Muggy";
    if (dp < 22.0f) return "Oppressive";
    return "Miserable";
}

static const char* classify_dew_spread(float spread) {
    if (spread < 2.5f) return "Fog Likely";
    return "Fog Unlikely";
}

static const char* classify_pressure(float pressure_hpa) {
    if (pressure_hpa >= 1030.0f) return "Very High";
    if (pressure_hpa >= 1020.0f) return "High";
    if (pressure_hpa >= 1010.0f) return "Normal";
    if (pressure_hpa >= 1000.0f) return "Low";
    if (pressure_hpa >=  990.0f) return "Very Low";
    if (pressure_hpa >=  980.0f) return "Extremely Low";
    return "Storm";
}

static const char* classify_pressure_trend(float trend_hpa_per_hour) {
    if (trend_hpa_per_hour >  1.2f) return "Clearing";
    if (trend_hpa_per_hour >  0.5f) return "Improving";
    if (trend_hpa_per_hour > -0.5f) return "Steady";
    if (trend_hpa_per_hour > -1.2f) return "Worsening";
    return "Storm Incoming";
}

static void print_padded(const char* text, uint32_t width) {
    Serial.print(text);
    uint32_t len = utf8_visible_len(text);
    if (len >= width) {
        Serial.print(' ');
        return;
    }
    for (uint32_t i = len; i < width; i++) {
        Serial.print(' ');
    }
}

// Counts display characters in a UTF-8 string by skipping continuation bytes.
// Fixes misalignment issues when using Celsius sign.
static uint32_t utf8_visible_len(const char* s) {
    uint32_t count = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) count++;
        s++;
    }
    return count;
}

static void print_row_header(const char* label) {
    print_padded(label, LABEL_WIDTH);
}

static void print_row_status(const char* status) {
    if (status == NULL || status[0] == '\0') {
        Serial.println();
        return;
    }
    Serial.print("  ");
    Serial.println(status);
}

static void print_value_padded(float value, uint8_t decimals, const char* unit) {
    char buf[24];
    int n = snprintf(buf, sizeof(buf), "%.*f", decimals, value);
    if (n < 0) n = 0;

    Serial.print(buf);
    Serial.print(' ');
    Serial.print(unit);

    uint32_t written = (uint32_t)n + 1 + utf8_visible_len(unit);
    for (uint32_t i = written; i < VALUE_WIDTH; i++) {
        Serial.print(' ');
    }
}
