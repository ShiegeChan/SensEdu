/*
 * Sensor read wrappers.
 *
 * Each function performs a single measurement and returns false on sensor error.
 */

bool measure_dps_temp(float* temp) {
    int16_t err = dps_sensor.measureTempOnce(*temp, DPS_OVERSAMPLING_RATE);
    if (err != 0) {
        Serial.print("Failed DPS Measurement. DPS lib error code: ");
        Serial.println(err);
        return false;
    }
    return true;
}

bool measure_dps_pressure_pa(float* pressure) {
    int16_t err = dps_sensor.measurePressureOnce(*pressure, DPS_OVERSAMPLING_RATE);
    if (err != 0) {
        Serial.print("Failed DPS Measurement. DPS lib error code: ");
        Serial.println(err);
        return false;
    }
    return true;
}

bool measure_sht_temp_humidity(float* temp, float* humidity) {
    int16_t err = sht_sensor.measureHighPrecision(*temp, *humidity);
    if (err != 0) {
        Serial.print("Failed SHT Measurement. SHT lib error code: ");
        Serial.println(err);
        return false;
    }
    return true;
}
