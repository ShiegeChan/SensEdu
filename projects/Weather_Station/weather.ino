/*
 * Weather math & rule-based prediction.
 *
 * Provides the sea-level pressure conversion and a coarse weather classifier
 * based on sea-level pressure zones with humidity sub-cases.
 */

// -----------------------------------------------
// Pressure zones (sea-level adjusted, hPa):
//  > 1030        = Strong anticyclone, reliably clear
//  1020–1030     = High pressure, fair but humidity can still bring cloud/light rain
//  1010–1020     = Variable/changeable zone (includes 1013.25 standard)
//  1000–1010     = Low pressure, rain likely
//   980–1000     = Deep low, wind and rain
//  < 980         = Storm / severe weather
// -----------------------------------------------
#define VERY_HIGH_HPA    1030
#define HIGH_HPA         1020
#define NORMAL_HPA       1010
#define LOW_HPA          1000
#define STORM_HPA         980

// Converts a pressure measurement at the given altitude to its equivalent
// sea-level pressure using the international barometric formula.
//
// Standard pressure at sea level is ~1013.25 hPa. Pressure decreases with
// altitude (less air above pressing down), so a station-level reading must
// be normalised before it can be compared against typical weather thresholds.
float calculate_sea_lvl_pressure_hpa(float pressure_pa, float temp, float altitude) {
    float sea_lvl_pa = pressure_pa * pow((1 - (0.0065 * altitude) / (temp + 273.15)), -5.257);
    return sea_lvl_pa / 100.0f;
}

void report_sea_lvl_pressure(float sea_lvl_pressure_hpa) {
    Serial.print("Equivalent Sea Level Pressure: ");
    Serial.print(sea_lvl_pressure_hpa);
    Serial.println(" hPa");
}

// Simple rule-based weather prediction driven primarily by sea-level pressure,
// with humidity selecting between sub-cases inside each pressure zone.
//
// Note: this only inspects a single instantaneous sample. A real predictor
// would track the rate of change of pressure over time (e.g. hPa/hour) by
// comparing past and current measurements.
void predict_weather(float pressure_hpa, float humidity) {

    if (pressure_hpa >= VERY_HIGH_HPA) {
        // Strong anticyclone: reliably stable.
        if (humidity < 65)       Serial.println("Weather: Clear / Sunny");
        else if (humidity < 85)  Serial.println("Weather: Hazy");
        else                     Serial.println("Weather: Foggy");
        return;
    }

    if (pressure_hpa >= HIGH_HPA) {
        // High but not fully settled: humidity matters a lot here.
        if (humidity < 55)       Serial.println("Weather: Fair");
        else if (humidity < 75)  Serial.println("Weather: Partly Cloudy / Light Rain possible");
        else                     Serial.println("Weather: Overcast / Rain possible");
        return;
    }

    if (pressure_hpa >= NORMAL_HPA) {
        // Changeable zone: precise decision requires pressure direction data and metrics trends.
        if (humidity < 60)       Serial.println("Weather: Changeable / Cloudy");
        else                     Serial.println("Weather: Rain likely");
        return;
    }

    if (pressure_hpa >= LOW_HPA) {
        // Proper low pressure: reliably stable.
        if (humidity < 65)       Serial.println("Weather: Cloudy / Showers");
        else                     Serial.println("Weather: Rain");
        return;
    }

    if (pressure_hpa >= STORM_HPA) {
        Serial.println("Weather: Heavy Rain / Wind");
        return;
    }

    Serial.println("Weather: Storm");
}
