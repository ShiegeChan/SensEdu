#include "SensEdu.h"

// Internal library error container
uint32_t lib_error = 0;

// Selected LED to blink
const uint8_t led = LED_BUILTIN;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    // Uncomment the loop to wait for you to start the Serial Monitor
    // This way you can see void setup logs
    Serial.begin(115200);
    //while (!Serial) {}

    SensEdu_TIMER_DelayInit();

    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);

    check_lib_errors();

    Serial.println("Setup is successful.");
}


/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    check_lib_errors();
    SensEdu_TIMER_Delay_us(500000);
    digitalWrite(led, HIGH);
    SensEdu_TIMER_Delay_us(500000);
    digitalWrite(led, LOW);
}

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
