#include "UltraSoundDrv.h"
//#include "src/UltraSoundDrv/src/UltraSoundDrv.h"

uint32_t lib_error = 0;
uint8_t led = LED_BUILTIN; // test with any digital pin (e.g. D2) or led (LED_BUILTIN)

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    // doesn't boot without opened serial monitor
    Serial.begin(115200);
    while (!Serial) {
        delay(1);
    }

    UltraSoundDrv_TIMER_Init();

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
    UltraSoundDrv_Delay_us(500000);
    digitalWrite(led, HIGH);
    UltraSoundDrv_Delay_us(500000);
    digitalWrite(led, LOW);
}

