/*
 * Weather Station
 *
 * Reads temperature, humidity and barometric pressure from an Infineon DPS368
 * and a Sensirion SHT40-AD1, both wired to the Arduino's primary I2C bus (Wire),
 * and prints a rule-based weather estimate over the serial port.
 *
 * I2C bus addresses: DPS368 @ 0x77, SHT40-AD1 @ 0x44.
 */

#include <Dps3xx.h>
#include <SensirionI2cSht4x.h>

// Set to 1 to block setup() until a serial monitor connects.
#define DEBUG_WAIT_FOR_SERIAL   0

/* -------------------------------------------------------------------------- */
/*                                User Settings                               */
/* -------------------------------------------------------------------------- */

// Location altitude in meters (501 m = Villach, Austria).
#define ALTITUDE_M                  501.0f

// Period between weather station state display cycles (seconds).
#define DISPLAY_PERIOD_SEC          5

// Period between pressure capture cycles (mins).
#define PRESSURE_PERIOD_MIN         30

// Pressure history buffer size.
#define PRESSURE_HISTORY_SIZE       48

// Sample window over which the pressure trend is calculated.
// Corresponds to (PRESSURE_TREND_SAMPLES * PRESSURE_PERIOD_MIN)
// time window in minutes.
#define PRESSURE_TREND_SAMPLES      6

// DPS oversampling exponent (0..7). The sensor performs 2^N internal
// measurements per result; higher values are more accurate but slower.
#define DPS_OVERSAMPLING_RATE       5

// Empirical temperature offset for DPS (in degrees Celsius).
#define DPS_T_OFFSET                0

// Empirical temperature offset for SHT (in degrees Celsius).
#define SHT_T_OFFSET                0

/* -------------------------------------------------------------------------- */
/*                                  Globals                                   */
/* -------------------------------------------------------------------------- */

SensirionI2cSht4x sht_sensor;
Dps3xx dps_sensor = Dps3xx();

/* -------------------------------------------------------------------------- */
/*                                   Setup                                    */
/* -------------------------------------------------------------------------- */

void setup() {
    Wire.begin();
    Serial.begin(9600);

#if DEBUG_WAIT_FOR_SERIAL
    while (!Serial);
#endif

    dps_sensor.begin(Wire);
    sht_sensor.begin(Wire, SHT40_I2C_ADDR_44);
    sht_sensor.softReset();
    delay(10);

    Serial.println("Init complete!");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    float sht_temperature      = 0.0f;
    float dps_temperature      = 0.0f;
    float pressure_pa          = 0.0f;
    float humidity             = 0.0f;
    float sea_lvl_pressure_hpa = 0.0f;

    measure_sht_temp_humidity(&sht_temperature, &humidity);
    measure_dps_temp(&dps_temperature);
    measure_dps_pressure_pa(&pressure_pa);

    sea_lvl_pressure_hpa = calculate_sea_lvl_pressure_hpa(pressure_pa, dps_temperature, ALTITUDE_M);
    report_sea_lvl_pressure(sea_lvl_pressure_hpa);

    try_save_pressure_meas(sea_lvl_pressure_hpa);
    report_pressure_trend();

    predict_weather(sea_lvl_pressure_hpa, humidity);

    delay(DISPLAY_PERIOD_SEC * 1000);
    Serial.println();
}
