/*
 * Read_RH_SHT
 *
 * Reads relative humidity from the Sensirion SHT40 sensor over I2C
 * and prints it to the Serial Monitor every second.
 *
 * I2C address: SHT40-AD1 @ 0x44
 */

#include <SensirionI2cSht4x.h>

SensirionI2cSht4x sht;

void setup() {
    Serial.begin(9600);
    while (!Serial);

    Wire.begin();
    sht.begin(Wire, SHT40_I2C_ADDR_44);
    sht.softReset();
    delay(10);

    Serial.println("SHT40 Humidity Reader Ready.");
}

void loop() {
    float temperature = 0.0f;
    float humidity = 0.0f;

    int16_t error = sht.measureHighPrecision(temperature, humidity);
    if (error != 0) {
        Serial.print("Measurement error: ");
        Serial.println(error);
    } else {
        Serial.print("Relative Humidity: ");
        Serial.print(humidity, 2);
        Serial.println(" %RH");
    }

    delay(1000);
}
