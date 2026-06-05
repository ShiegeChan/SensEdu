/*
 * Read_Temp_DPS
 *
 * Reads temperature from the Infineon DPS368 pressure sensor over I2C
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

    Serial.println("DPS368 Temperature Reader Ready.");
}

void loop() {
    float temperature = 0.0f;

    int16_t error = dps368.measureTempOnce(temperature, 5);
    if (error != 0) {
        Serial.print("Measurement error: ");
        Serial.println(error);
    } else {
        Serial.print("Temperature: ");
        Serial.print(temperature, 2);
        Serial.println(" °C");
    }

    delay(1000);
}
