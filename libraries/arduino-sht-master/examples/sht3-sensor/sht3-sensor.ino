#include <Wire.h>
#include "SHTSensor.h"

// Note that all i2c devices sharing one bus must have distinct addresses. Thus
// this example only works with sensors that have distinct i2c addresses (e.g.
// SHT3x and SHT3x_alt, or SHT3x and SHTC3).
// Make sure not to use auto-detection as it will only pick up one sensor.

// Sensor with normal i2c address
// Sensor 1 with address pin pulled to GND
SHTSensor sht1(SHTSensor::SHT3X);

void setup() {
  // put your setup code here, to run once:
  Wire1.begin();
  Serial.begin(9600);
  delay(1000); // let serial console settle

  // init on a specific sensor type (i.e. without auto detecting),
  // does not check if the sensor is responding and will thus always succeed.

  // initialize sensor with normal i2c-address
  sht1.init();


}

void loop() {
  // put your main code here, to run repeatedly:
  // read from first sensor
  if (sht1.readSample()) {
    Serial.print("SHT1 :\n");
    Serial.print("  RH: ");
    Serial.print(sht1.getHumidity(), 2);
    Serial.print("\n");
    Serial.print("  T:  ");
    Serial.print(sht1.getTemperature(), 2);
    Serial.print("\n");
  } else {
    Serial.print("Sensor 1: Error in readSample()\n");
  }


  delay(1000);
}
