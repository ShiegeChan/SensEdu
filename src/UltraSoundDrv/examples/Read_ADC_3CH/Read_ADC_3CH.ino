#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_adc.h"
#include "stm32h747xx.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_rcc.h"

#include "UltraSoundDrv.h"

uint32_t error = 0;

#define ADC_RESOLUTION 		16   	// 8, 10, 12, 14, 16
#define ADC_DIFF           	0      	// A10 has to be used as input positive on giga r1, A11 as negative
#define ADC_CLK_FREQ        40     	// Clock speed in mhz, stable up to 40mhz
#define ADC_SAMPLE_TIME 	0       // 0 to 7: 0 is minimal sampling time. If you change these, recalculate sampling rate in matlab
#define ADC_SAMPLE_NUM    	0       // oversampling: 0 - OFF


void print_PLL1_Config() {
    Serial.print("DIVP1: ");
    Serial.println(READ_BIT(RCC->PLL1DIVR, RCC_PLL1DIVR_P1) >> RCC_PLL1DIVR_P1_Pos); // 1 -> 2
    Serial.print("DIVN1: ");
    Serial.println(READ_BIT(RCC->PLL1DIVR, RCC_PLL1DIVR_N1) >> RCC_PLL1DIVR_N1_Pos); // 119 -> 120
    Serial.print("DIVM1: ");
    Serial.println(READ_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM1) >> RCC_PLLCKSELR_DIVM1_Pos); // 2 -> 2
    Serial.print("D1CPRE: ");
    Serial.println(READ_BIT(RCC->D1CFGR, RCC_D1CFGR_D1CPRE) >> RCC_D1CFGR_D1CPRE_Pos); // 0 -> bypass

    // ARDUINO USES 16MHz CLOCK AS INPUT TO PLL as HSE
}

const uint8_t array_size = 3;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200); // 14400 bytes/sec -> 7200 samples/sec -> 2400 samples/sec for 1 mic
    
    while (!Serial) {
        delay(1);
    }

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("start init");

    uint8_t adc_pins[array_size] = {A0,A1,A2};

    UltraSoundDrv_Init(ADC1, adc_pins, array_size, true, 1100);
    UltraSoundDrv_ADC_Enable(ADC1);
    UltraSoundDrv_ADC_Start(ADC1);
    //ADCBegin(ADC1, A5, ADC_RESOLUTION, ADC_DIFF, ADC_CLK_FREQ, ADC_SAMPLE_TIME, ADC_SAMPLE_NUM);

    /*
    Serial.println("--");
    Serial.println(READ_BIT(RCC->CR, RCC_CR_HSIDIV)); // 00
    Serial.println(READ_BIT(RCC->CFGR, RCC_CFGR_MCO2)); // 000
    Serial.println(READ_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_PLLSRC)); // 10
    print_PLL1_Config();
    Serial.println("--");
    */

    //CLEAR_BIT(SYSCFG->PMCR, SYSCFG_PMCR_PC3SO); short _C pin to usual

    Serial.print("msg: ");
    Serial.println(get_msg());

    error = UltraSoundDrv_GetError();
    while (error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(error, HEX);
    }

    Serial.print("ARR reg: ");
    Serial.println(TIM1->ARR);
    Serial.print("Sampling freq: ");
    Serial.println(10000000/(TIM1->ARR));

    delay(2000);
}

void loop() {
    Serial.println("-----");

    //improve reading function
    uint16_t* temp = UltraSoundDrv_ADC_Read(ADC1);
    for (uint8_t i = 0; i < array_size; i++) {
        Serial.print("Value CH");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(temp[i]);
    }
}
