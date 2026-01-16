#include <Wire.h> // The I²C library for Arduino

void setup() {
  Wire1.begin(); // Initialize the I²C bus
  Serial.begin(9600); // Start Serial communication
  while (!Serial); // Wait for Serial Monitor to open (useful for boards like Arduino Leonardo, Micro, etc.)
  Serial.println("I2C Scanner");
}

void loop() {
  byte error, address;  
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  
  for (address = 1; address < 127; address++) { // I2C addresses are 7 bits (0x01 to 0x7F)
    // Start communication with the I²C device at the current address
    Wire1.beginTransmission(address);
    error = Wire1.endTransmission();

    // Check if a device acknowledged our request
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX); // Prints address in hex format
      Serial.println(" !");
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("Done.\n");
  }

  delay(5000); // Wait 5 seconds before scanning again
}