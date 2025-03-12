#include "SensEdu.h"

uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 3;
uint8_t adc_pins[adc_pin_num] = {A0, A1, A2};
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT,
    .sampling_freq = 0,
    
    .dma_mode = SENSEDU_ADC_DMA_DISCONNECT,
    .mem_address = 0x0000,
    .mem_size = 0
};
uint8_t led = D2; // test with any digital pin (e.g. D2) or led (LED_BUILTIN)

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    // doesn't boot without opened serial monitor
    Serial.begin(115200);
    while (!Serial) {
        delay(1);
    }
    Serial.println("Started Initialization...");
    
    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
    SensEdu_ADC_Start(adc);

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);

    Serial.println("Setup is successful.");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    // use one shot
    // to simplify synchronization in multi-channel mode
    uint16_t* data = SensEdu_ADC_ReadSequence(adc);
    
    Serial.println("-------");
    for (uint8_t i = 0; i < adc_pin_num; i++) {
        Serial.print("Value CH");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(data[i]);
    }

    if (digitalRead(led) == HIGH) {
        digitalWrite(led, LOW);
    } else {
        digitalWrite(led, HIGH);
    }
}
