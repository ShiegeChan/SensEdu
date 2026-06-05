/*
 * Adapted from Sensirion's exampleUsage.ino for the SHT4x sensor.
 *
 * Local fix: the original example uses a file-scope variable named `error`,
 * which clashes with `mbed::error()` declared in the Mbed Arduino core
 * (Giga, Portenta, Nano 33 BLE, Nano RP2040). Renamed to `sht_error` to
 * resolve the collision.
 */
#include <Arduino.h>
#include <SensirionI2cSht4x.h>
#include <Wire.h>

// Make sure we use the proper definition of NO_ERROR
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

SensirionI2cSht4x sensor;

static char    errorMessage[64];
static int16_t sht_error;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }
    Wire.begin();
    sensor.begin(Wire, SHT40_I2C_ADDR_44);

    sensor.softReset();
    delay(10);

    uint32_t serialNumber = 0;
    sht_error = sensor.serialNumber(serialNumber);
    if (sht_error != NO_ERROR) {
        Serial.print("Error trying to execute serialNumber(): ");
        errorToString(sht_error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
    Serial.print("serialNumber: ");
    Serial.print(serialNumber);
    Serial.println();
}

void loop() {
    float aTemperature = 0.0f;
    float aHumidity    = 0.0f;

    delay(20);

    sht_error = sensor.measureLowestPrecision(aTemperature, aHumidity);
    if (sht_error != NO_ERROR) {
        Serial.print("Error trying to execute measureLowestPrecision(): ");
        errorToString(sht_error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
    Serial.print("aTemperature: ");
    Serial.print(aTemperature);
    Serial.print("\t");
    Serial.print("aHumidity: ");
    Serial.print(aHumidity);
    Serial.println();
}
