#include "SensEdu.h"

uint32_t lib_error = 0;
uint8_t pwm_chs[4] = {D4, D37, D48, D71};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);
    Serial.println("Started Initialization...");

    SensEdu_PWM_Init(pwm_chs[0], 100000, 25);
    SensEdu_PWM_Init(pwm_chs[1], 100000, 50);
    SensEdu_PWM_Init(pwm_chs[2], 100000, 75);
    SensEdu_PWM_Init(pwm_chs[3], 100000, 100);
    SensEdu_PWM_Start();

    check_lib_errors();

    Serial.println("Setup is successful.");
}


/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    check_lib_errors();
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
