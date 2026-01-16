#include "SensEdu.h"
#include "SineLUT.h"
#include <WiFi.h>

// Internal library error container
uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

#define WIFI_SSID   "TestWiFi"
#define WIFI_PASS   "TestWiFi"
#define WIFI_PORT   80

/* DAC */
// LUT settings are in SineLUT.h
#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle

DAC_Channel* dac_ch = DAC_CH1;
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = dac_ch, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)sine_lut,
    .mem_size = sine_lut_size,
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = dac_cycle_num
};

/* ADC */
const uint16_t mic_data_size = 2000;
SENSEDU_ADC_BUFFER(mic_data, mic_data_size);

ADC_TypeDef* adc = ADC1;
const uint8_t mic_num = 1;
uint8_t mic_pins[mic_num] = {A1};
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = mic_pins,
    .pin_num = mic_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 250000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)mic_data,
    .mem_size = mic_data_size
};

/* WiFi */
int status = WL_IDLE_STATUS;
WiFiServer server(WIFI_PORT);

const uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);

    SensEdu_DAC_Init(&dac_settings);

    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);

    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    check_lib_errors();

    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(WIFI_SSID);

        // Connect to WPA/WPA2 network (change this if youre using open / WEP network)
        status = WiFi.begin(WIFI_SSID, WIFI_PASS);

        // Wait 10 seconds for connection:
        delay(10000);
    }
    server.begin();
    print_wifi_status();
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    WiFiClient client = server.available();
    if (!client) {
        return;
    }
    Serial.println("Client connected!");

    // Measurement is initiated by the signal from computing device (matlab script)
    static char buf = 0;

    while (client.connected()) {
        if (!client.available()) {
            continue;
        }

        buf = client.read();
        if (buf != 't') { // Trigger not detected
            continue;
        }
            
        // Start dac->adc sequence
        SensEdu_DAC_Enable(dac_ch);
        while (!SensEdu_DAC_GetBurstCompleteFlag(dac_ch));
        SensEdu_DAC_ClearBurstCompleteFlag(dac_ch);
        SensEdu_ADC_Start(adc);
        
        // Wait for the data and send it
        while (!SensEdu_ADC_IsDmaTransferComplete(adc));
        SensEdu_ADC_ClearDmaTransferComplete(adc);
        wifi_send_array(client, (const uint8_t *) & mic_data, mic_data_size << 1);

        check_lib_errors();
    }
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

// Checks if the library has risen any internal errors
// Prints the error code in Serial Monitor
void check_lib_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}

void wifi_send_array(WiFiClient client, const uint8_t* data, size_t size) {
    client.write(data, size);
}

void print_wifi_status(void) {
    // print the SSID of the network youre connected to
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your boards local IP address
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received WiFi signal strength
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}
