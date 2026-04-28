#include <Wire.h>

#define SERIAL_BAUD         9600U
#define I2C_ADDR_MIN        0x01
#define I2C_ADDR_MAX        0x7F
#define SCAN_INTERVAL_MS    5000U

// Status codes returned by Wire.endTransmission()
typedef enum {
    I2C_OK                = 0,
    I2C_ERR_DATA_TOO_LONG = 1,
    I2C_ERR_ADDR_NACK     = 2,
    I2C_ERR_DATA_NACK     = 3,
    I2C_ERR_OTHER         = 4
} I2CStatus;

void setup() {
    Wire.begin();
    Serial.begin(SERIAL_BAUD);
    while (!Serial);
    Serial.println("I2C Scanner");
}

void loop() {
    Serial.println("Scanning...");

    const uint8_t devices_found = scan_bus();

    if (devices_found == 0) {
        Serial.println("No I2C devices found\n");
    } else {
        Serial.println("Done.\n");
    }

    delay(SCAN_INTERVAL_MS);
}

static uint8_t scan_bus() {
    uint8_t devices_found = 0;

    // Iterate through 7-bit addresses
    for (uint8_t address = I2C_ADDR_MIN; address < I2C_ADDR_MAX; ++address) {
        I2CStatus error = I2C_OK;

        if (probe_address(address, error)) {
            Serial.print("I2C device found at address 0x");
            print_hex_byte(address);
            Serial.println(" !");
            ++devices_found;
        } else if (error == I2C_ERR_OTHER) {
            Serial.print("Unknown error at address 0x");
            print_hex_byte(address);
            Serial.println();
        }
    }

    return devices_found;
}

static void print_hex_byte(uint8_t value) {
    if (value < 0x10) Serial.print('0');
    Serial.print(value, HEX);
}

static bool probe_address(uint8_t address, I2CStatus& error_out) {
    Wire.beginTransmission(address);
    error_out = static_cast<I2CStatus>(Wire.endTransmission());
    return error_out == I2C_OK;
}