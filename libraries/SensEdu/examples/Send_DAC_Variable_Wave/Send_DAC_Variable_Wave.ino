#include <SensEdu.h>

uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

const size_t lut_size = 17;
static SENSEDU_DAC_BUFFER(lut, lut_size) = {
    0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,0x0010
};
static uint8_t increment_flag = 1; // run time modification flag

void disable_dac_cache()
{
    LL_MPU_Disable();
    LL_MPU_ConfigRegion(LL_MPU_REGION_NUMBER5, 0x0, (uint32_t)(&lut), LL_MPU_REGION_SIZE_64B|LL_MPU_TEX_LEVEL1|LL_MPU_REGION_FULL_ACCESS|LL_MPU_INSTRUCTION_ACCESS_DISABLE|LL_MPU_ACCESS_SHAREABLE|LL_MPU_ACCESS_NOT_CACHEABLE|LL_MPU_ACCESS_NOT_BUFFERABLE);
    LL_MPU_EnableRegion(LL_MPU_REGION_NUMBER5);
    SCB_CleanDCache_by_Addr((uint32_t*)lut, 64);
    LL_MPU_Enable(LL_MPU_CTRL_PRIVILEGED_DEFAULT);
}


/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    Serial.begin(115200);

    disable_dac_cache();
    SensEdu_DAC_Settings dac1_settings = {DAC1, 64000*16, (uint16_t*)lut, lut_size, 
        SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0};

    SensEdu_DAC_Init(&dac1_settings);
    SensEdu_DAC_Enable(DAC1);

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    Serial.println("Setup is successful.");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    // modify lut
    for (uint16_t i = 0; i < lut_size; i++) {
        if (increment_flag) {
            lut[i]++;
        } else {
            lut[i]--;
        }
    }

    // out of bounds checks
    if (lut[0] == 0x0000) {
        increment_flag = 1;
    }
    if (lut[lut_size-1] == 0x0FFF) {
        increment_flag = 0;
    }
    
    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}
