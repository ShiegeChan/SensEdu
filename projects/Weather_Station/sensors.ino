/*
 * Sensor read wrappers.
 *
 * Each function performs a single measurement, applies the empirical offset
 * (where applicable), prints the value to the serial port, and reports any
 * library error code. Sensor objects and offset constants are defined in
 * Weather_Station.ino.
 */

void measure_dps_temp(float* temp) {
    int16_t err = dps_sensor.measureTempOnce(*temp, DPS_OVERSAMPLING_RATE);
    if (err != 0) {
        Serial.print("FAIL! DPS lib error code: ");
        Serial.println(err);
        return;
    }
    *temp += DPS_T_OFFSET;

    Serial.print("Temperature (dps): ");
    Serial.print(*temp);
    Serial.println("°C");
}

void measure_dps_pressure_pa(float* pressure) {
    int16_t err = dps_sensor.measurePressureOnce(*pressure, DPS_OVERSAMPLING_RATE);
    if (err != 0) {
        Serial.print("FAIL! DPS lib error code: ");
        Serial.println(err);
        return;
    }

    Serial.print("Pressure: ");
    Serial.print(*pressure / 100.0f);
    Serial.println(" hPa");
}

void measure_sht_temp_humidity(float* temp, float* humidity) {
    int16_t err = sht_sensor.measureHighPrecision(*temp, *humidity);
    if (err != 0) {
        Serial.print("FAIL! SHT lib error code: ");
        Serial.println(err);
        return;
    }
    *temp += SHT_T_OFFSET;

    Serial.print("Humidity level: ");
    Serial.print(*humidity, 2);
    Serial.println("%");

    Serial.print("Temperature (sht): ");
    Serial.print(*temp, 2);
    Serial.println("°C");
}
