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
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static const char* classify_temperature(float temp);
static const char* classify_humidity(float humidity);
static const char* classify_dew_spread(float spread);
static const char* classify_pressure(float pressure_hpa);
static const char* classify_pressure_trend(float trend_hpa_per_hour);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

// Converts a pressure measurement at the given altitude to its equivalent sea-level pressure.
//
// Standard pressure at sea level is ~1013.25 hPa. Pressure decreases with altitude.
// A station-level reading must be normalised before it can be compared against typical weather thresholds.
float calculate_sea_lvl_pressure_hpa(float pressure_pa, float temp, float altitude) {
    float sea_lvl_pa = pressure_pa * pow((1 - (0.0065 * altitude) / (temp + 273.15)), -5.257);
    return sea_lvl_pa / 100.0f;
}

// Calculates dew point based on Magnus formula.
float calculate_dew_point(float temp, float humidity) {
    const float a = 17.625f;
    const float b = 243.04f;
    float gamma = (a * temp) / (b + temp) + log(humidity / 100.0f);
    return (b * gamma) / (a - gamma);
}

void report_temp(float temp) {
    Serial.print("Temperature: ");
    Serial.print(temp, 2);
    Serial.print(" °C (");
    Serial.print(classify_temperature(temp));
    Serial.println(")");
}

void report_humidity(float humidity) {
    Serial.print("Humidity: ");
    Serial.print(humidity, 2);
    Serial.print(" % (");
    Serial.print(classify_humidity(humidity));
    Serial.println(")");
}

void report_dew_spread(float spread) {
    Serial.print("Dew Point Spread: ");
    Serial.print(spread, 2);
    Serial.print(" °C (");
    Serial.print(classify_dew_spread(spread));
    Serial.println(")");
}

void report_altitude(float altitude) {
    Serial.print("Altitude: ");
    Serial.print(altitude);
    Serial.println(" m");
}

void report_pressure(float pressure_hpa) {
    Serial.print("Measured Pressure: ");
    Serial.print(pressure_hpa, 2);
    Serial.println(" hPa");
}

void report_sea_level_pressure(float pressure_hpa) {
    Serial.print("Adjusted Pressure: ");
    Serial.print(pressure_hpa, 2);
    Serial.print(" hPa (");
    Serial.print(classify_pressure(pressure_hpa));
    Serial.println(")");
}

void report_pressure_trend(void) {
    PressureTrendStatus status;
    get_pressure_trend_status(&status);

    if (!status.trend_available) {
        Serial.print("Pressure Trend: collecting data (");
        Serial.print(status.samples_captured);
        Serial.print("/");
        Serial.print(status.samples_required);
        Serial.println(" samples)");
    } else {
        Serial.print("Pressure Trend: ");
        Serial.print(status.trend_hpa_per_hour, 2);
        Serial.print(" hPa/h (");
        Serial.print(classify_pressure_trend(status.trend_hpa_per_hour));
        Serial.println(")");
    }

    uint32_t remaining_min = status.ms_until_next_capture / 60000UL;
    Serial.print("Next trend pressure sample in: ");
    Serial.print(remaining_min / 60);
    Serial.print(":");
    uint32_t mm = remaining_min % 60;
    if (mm < 10) Serial.print('0');
    Serial.println(mm);
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */

static const char* classify_temperature(float temp) {
    if (temp < -10.0f)  return "very cold";
    if (temp <  0.0f)   return "cold";
    if (temp <  10.0f)  return "cool";
    if (temp <  20.0f)  return "mild";
    if (temp <  27.0f)  return "warm";
    if (temp <  35.0f)  return "hot";
    return "very hot";
}

static const char* classify_humidity(float humidity) {
    if (humidity < 30.0f) return "dry";
    if (humidity < 60.0f) return "comfortable";
    if (humidity < 75.0f) return "humid";
    if (humidity < 90.0f) return "very humid";
    return "saturated";
}

static const char* classify_dew_spread(float spread) {
    if (spread < 2.5f)  return "fog likely";
    if (spread < 5.0f)  return "mist possible";
    if (spread < 10.0f) return "comfortable";
    return "dry air";
}

static const char* classify_pressure(float pressure_hpa) {
    if (pressure_hpa >= 1030.0f) return "very high";
    if (pressure_hpa >= 1020.0f) return "high";
    if (pressure_hpa >= 1010.0f) return "normal";
    if (pressure_hpa >= 1000.0f) return "low";
    if (pressure_hpa >=  990.0f) return "very low";
    return "extremely low";
}

static const char* classify_pressure_trend(float trend_hpa_per_hour) {
    if (trend_hpa_per_hour >  2.0f) return "rising fast - weather clearing soon";
    if (trend_hpa_per_hour >  0.5f) return "rising - weather improving";
    if (trend_hpa_per_hour > -0.5f) return "steady - weather steady";
    if (trend_hpa_per_hour > -2.0f) return "falling - weather worsening";
    return "falling fast - storm approaching";
}
