/*
 * Read_Pressure_DPS
 *
 * Reads barometric pressure from the Infineon DPS368 sensor over I2C
 * and prints it to the Serial Monitor every second.
 *
 * I2C address: DPS368 @ 0x77
 */

#include <Dps3xx.h>

Dps3xx dps368;

void setup() {
    Serial.begin(9600);
    while (!Serial);

    Wire.begin();
    dps368.begin(Wire);

    Serial.println("DPS368 Pressure Reader Ready.");
}

void loop() {
    float pressure = 0.0f;

    int16_t error = dps368.measurePressureOnce(pressure, 5);
    if (error != 0) {
        Serial.print("Measurement error: ");
        Serial.println(error);
    } else {
        Serial.print("Pressure: ");
        Serial.print(pressure, 2);
        Serial.println(" Pa");
    }

    delay(1000);
}
