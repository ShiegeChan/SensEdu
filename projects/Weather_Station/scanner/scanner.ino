// I2C bus scanner for the SensEdu Weather Station.
// Walks both Wire and Wire1 and prints every responding 7-bit address.
// Designed to detect the DPS368 pressure sensor and the SHT4x humidity sensor

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
    I2C_ERR_OTHER         = 4,
    I2C_ERR_TIMEOUT       = 5
} I2CStatus;

static void    print_hex_byte(uint8_t value);
static bool    probe_address(TwoWire& bus, uint8_t address, I2CStatus& error_out);
static uint8_t scan_bus(TwoWire& bus);

void setup() {
    Wire.begin();
    Wire1.begin();

    Serial.begin(SERIAL_BAUD);
    while (!Serial);

    Serial.println("I2C Scanner");
}

void loop() {
    Serial.println("Scanning Wire (I2C0)...");
    const uint8_t found_wire  = scan_bus(Wire);

    Serial.println("Scanning Wire1 (I2C1)...");
    const uint8_t found_wire1 = scan_bus(Wire1);

    const uint8_t found_total = found_wire + found_wire1;
    if (found_total == 0) {
        Serial.println("No I2C devices found.\n");
    } else {
        Serial.print("Done. ");
        Serial.print(found_total);
        Serial.println(" device(s) total.\n");
    }

    delay(SCAN_INTERVAL_MS);
}

static void print_hex_byte(uint8_t value) {
    if (value < 0x10) Serial.print('0');
    Serial.print(value, HEX);
}

// Probes a single address.
// Writes a single dummy byte and accepts either I2C_OK (device ACKed everything) or
// I2C_ERR_DATA_NACK (device ACKed its address but NACKed the payload) as "device present".
//
// Uses 1-byte write instead of the classic 0-byte probe, because SHT4x is command-driven sensor.
// DPS368 for instance, could be detected in a classic way.
static bool probe_address(TwoWire& bus, uint8_t address, I2CStatus& error_out) {
    bus.beginTransmission(address);
    bus.write(static_cast<uint8_t>(0x00));
    error_out = static_cast<I2CStatus>(bus.endTransmission());
    return error_out == I2C_OK || error_out == I2C_ERR_DATA_NACK;
}

static uint8_t scan_bus(TwoWire& bus) {
    uint8_t devices_found = 0;

    for (uint8_t address = I2C_ADDR_MIN; address < I2C_ADDR_MAX; ++address) {
        I2CStatus error = I2C_OK;

        if (probe_address(bus, address, error)) {
            Serial.print("  I2C device found at address 0x");
            print_hex_byte(address);
            Serial.println("");
            ++devices_found;
        } else if (error == I2C_ERR_OTHER || error == I2C_ERR_TIMEOUT) {
            Serial.print("  Bus error (");
            Serial.print(error);
            Serial.print(") at address 0x");
            print_hex_byte(address);
            Serial.println();
        }
    }

    return devices_found;
}