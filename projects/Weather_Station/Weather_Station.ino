#include <Dps3xx.h>
#include "SHTSensor.h"

/**
 * @details This example reads temperature and pressure values and estimates
 *          weather conditions based on equivalent sea level pressure
 */

/* -------------------------------------------------------------------------- */
/*                                User Settings                               */
/* -------------------------------------------------------------------------- */

#define NORMAL_PRESSURE             1009            // in hPa
#define HIGH_PRESSURE               1022            // in hPa
#define T_OFFSET                    0              // Temperature offset for accurate temperature display (empirical)
#define g                           9.81            // m/s^2
#define P_0                         101325          // Pa
#define M                           0.02896         // kg/mol
#define R                           8.314           // J/molK

/* -------------------------------------------------------------------------- */
/*                                 Parameters                                 */
/* -------------------------------------------------------------------------- */

float altitude = 0;             // Altitude in meters (user-defined)
float temperature;
float pressure;
float seaLevelPressure;
float humidity;
int16_t ret;

// Dps3xx Object
Dps3xx dps_sensor = Dps3xx();
uint8_t oversampling = 5;       /* Value from 0 to 7, the Dps 3xx will perform 2^oversampling internal
    temperature measurements and combine them to one result with higher precision
    measurements with higher precision take more time, consult datasheet for more information */

SHTSensor sht_sensor(SHTSensor::SHT3X);

/* -------------------------------------------------------------------------- */
/*                                 Functions                                  */
/* -------------------------------------------------------------------------- */

// Function to calculate sea-level pressure
/* Standard atmospheric pressure at sea level is approximately 1013.25 hectopascals (hPa),
29.92 inches of mercury (inHg), or 14.7 pounds per square inch (psi) */

// Air pressure decreases with increasing altitude. 
// As you move higher into the atmosphere, there is less air above you pressing down, 
// resulting in lower pressure. This is why mountain climbers often experience difficulty 
// breathing due to the thinner air and lower oxygen levels.

float calculateSeaLevelPressure(float p, float t) {
    return p * pow((1 - (0.0065 * altitude) / (t + 273.15)), -5.257);
}

// Simple Weather Prediction Rule-Based Model 

/* This function is not implemented correctly yet as it only predicts weather based on
instant measurement. It should measure the rate of change in pressure over time
(e.g. in hPa/hour) to predict weather. The rate should be measured by comparing an
initial pressure measurement with the current measurement.
This function should also include a humidity parameter from the humidity sensor to
help predict weather more accurately*/

void WeatherPredict(float temp, float pressure, float humidity) {
    // temp -> in degreees 
    // pressure -> in hPa (hectopascal is 100 pascals)
    // humidity -> in % 

    if (humidity > 70 && pressure < 1000 && (15 < temp < 25)) {
        Serial.println("Weather Status: Rain"); 
    }
    else if ((50 <= humidity <= 70) && pressure < 1000) {
        Serial.println("Weather Status: Cloudy");
    } 
    else if (pressure > 1020 && humidity < 50 && temp > 20) {
        Serial.println("Weather Status: Clear");
    } 
    else {
        Serial.println("Weather Status: Uncertain");
    }
}

/* -------------------------------------------------------------------------- */
/*                                   Setup                                    */
/* -------------------------------------------------------------------------- */

void setup() {
    Wire1.begin();
    Serial.begin(9600);
    while (!Serial); // Wait for Serial Monitor to connect

    dps_sensor.begin(Wire1); 
    sht_sensor.init(Wire1); 

    Serial.println("Init complete!");

    // Ask the user to input the altitude
    Serial.println("Please input the current altitude (in meters):");

    // Wait for user input via Serial Monitor
    while (Serial.available() == 0) {
        // Do nothing, wait for the user to input data
    }

    // Read the user input and convert it to a float
    String input = Serial.readStringUntil('\n');
    altitude = input.toFloat();

    // Confirm the altitude
    Serial.print("Altitude set to: ");
    Serial.print(altitude);
    Serial.println(" meters");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop()
{
    Serial.println();

    // ----------------------------- temperature measurement -------------------------

    ret = dps_sensor.measureTempOnce(temperature, oversampling);
    if (ret != 0) {
        Serial.print("FAIL! ret = ");
        Serial.println(ret);
    }
    else {
        Serial.print("Temperature (dps): ");
        Serial.print(temperature + T_OFFSET);
        Serial.println("°C");
    }

    // ----------------------------- pressure measurement -------------------------

    ret = dps_sensor.measurePressureOnce(pressure, oversampling);
    if (ret != 0) {
        // Something went wrong.
        // Look at the library code for more information about return codes
        Serial.print("FAIL! ret = ");
        Serial.println(ret);
    }
    else {
        Serial.print("Pressure: ");
        Serial.print(pressure/100);
        Serial.println(" hPa");
    }

    seaLevelPressure = calculateSeaLevelPressure(pressure, temperature); 
    Serial.print("Equivalent Sea Level Pressure: ");
    Serial.print(seaLevelPressure / 100);
    Serial.println(" hPa");


// ----------------------------- humidity measurement -------------------------
    if (sht_sensor.readSample()) {
        humidity = sht_sensor.getHumidity();
        Serial.print("Humidity level: ");
        Serial.println(humidity, 2);
        Serial.print("Temperature (sht)");
        Serial.println(sht_sensor.getTemperature(), 2);
    } 
    else {
        Serial.println("SHT sensor Error\n");
    }

    WeatherPredict(temperature, pressure, humidity);

    // Wait some time
    delay(10000);
}
