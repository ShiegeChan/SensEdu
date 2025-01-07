#include "UltraSoundDrv.h"

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() { 
    UltraSoundDrv_TIMER_Init();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
}


/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    UltraSoundDrv_Delay_us(500000);
    digitalWrite(LED_BUILTIN, HIGH);
    UltraSoundDrv_Delay_us(500000);
    digitalWrite(LED_BUILTIN, LOW);
}

