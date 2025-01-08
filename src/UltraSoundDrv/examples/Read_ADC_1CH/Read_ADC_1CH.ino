#include "UltraSoundDrv.h"
//#include "src/UltraSoundDrv/src/UltraSoundDrv.h"

uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 1;
uint8_t adc_pins[adc_pin_num] = {A0};

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
    
    UltraSoundDrv_Init(adc, adc_pins, adc_pin_num, false, 1000); // timer triggering is off and frequency is ignored
    UltraSoundDrv_ADC_Enable(adc);
    UltraSoundDrv_ADC_Start(adc);

    lib_error = UltraSoundDrv_GetError();
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
    uint16_t* temp = UltraSoundDrv_ADC_Read(adc);
    Serial.println(temp[0]);

    if (digitalRead(led) == HIGH) {
        digitalWrite(led, LOW);
    } else {
        digitalWrite(led, HIGH);
    }

}
