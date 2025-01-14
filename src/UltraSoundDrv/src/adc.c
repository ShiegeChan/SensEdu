#include "adc.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */
typedef struct {
    uint8_t number;
    uint32_t preselection;
} channel;


/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static ADC_ERROR error = ADC_ERROR_NO_ERRORS;
static uint8_t adc_flag = 0;

uint8_t adc_msg = 0;

ADC_Settings ADC1_Settings = {0, 0, ULTRASOUND_DRV_ADC_MODE_ONE_SHOT, 0, 0};
ADC_Settings ADC2_Settings = {0, 0, ULTRASOUND_DRV_ADC_MODE_ONE_SHOT, 0, 0};


/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void configure_pll2(void);
void adc_init(ADC_TypeDef* ADC, uint8_t* arduino_pins, uint8_t adc_pin_num, ULTRASOUND_DRV_ADC_MODE mode);
channel get_adc_channel(uint8_t arduino_pin, ADC_TypeDef* ADC);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
ADC_ERROR ADC_GetError(void) {
    return error;
}

void ADC_InitPeriph(ADC_TypeDef* ADC, uint8_t* arduino_pins, uint8_t adc_pin_num, ULTRASOUND_DRV_ADC_MODE mode) {
    if (ADC != ADC1 && ADC != ADC2) {
        error = ADC_ERROR_WRONG_ADC; // you can only use ADC1 or ADC2
        return;
    }

    ADC_GetSettings(ADC)->conv_length = adc_pin_num;
    ADC_GetSettings(ADC)->adc_pins = arduino_pins;
    ADC_GetSettings(ADC)->eoc_flag = 0;
    ADC_GetSettings(ADC)->mode = mode;

    configure_pll2();
    adc_init(ADC, arduino_pins, adc_pin_num, mode);

    // End of conversion of a regular group EOC EOCIE
}

void ADC_EnablePeriph(ADC_TypeDef* ADC) {
    // clear ready bit
    SET_BIT(ADC->ISR, ADC_ISR_ADRDY);
    
    // enable ADC
    SET_BIT(ADC->CR, ADC_CR_ADEN);
    while(!READ_BIT(ADC->ISR, ADC_ISR_ADRDY));

    // check if ready to start
    if ((!READ_BIT(ADC->CR, ADC_CR_ADEN) | READ_BIT(ADC->CR, ADC_CR_ADDIS))) {
        error = ADC_ERROR_ADC_ENABLE_FAIL;
    }
}

void ADC_DisablePeriph(ADC_TypeDef* ADC) {
    // check if conversion is ongoing
    if (READ_BIT(ADC->CR, ADC_CR_ADSTART)) {
        SET_BIT(ADC->CR, ADC_CR_ADSTP); // stop conversion
        while(READ_BIT(ADC->CR, ADC_CR_ADSTP)); // wait till it is stopped
    }

    if (READ_BIT(ADC->CR, ADC_CR_ADSTART)) {
        error = ADC_ERROR_ADC_DISABLE_FAIL;
    }

    SET_BIT(ADC->CR, ADC_CR_ADDIS);
    while(READ_BIT(ADC->CR, ADC_CR_ADEN));
}

void ADC_StartConversion(ADC_TypeDef* ADC) {
    //while (!READ_REG(ADC->ISR & ADC_ISR_ADRDY));

    SET_BIT(ADC->CR, ADC_CR_ADSTART);
}

uint16_t* ADC_ReadSingleSequence(ADC_TypeDef* ADC) {
    ADC_Settings* settings = ADC_GetSettings(ADC);

    for (uint8_t i = 0; i < settings->conv_length; i++) {
        settings->eoc_flag = 1;
        while(settings->eoc_flag);
        settings->sequence_data[i] = READ_REG(ADC->DR);
    }
    
    return settings->sequence_data;
}

ADC_Settings* ADC_GetSettings(ADC_TypeDef* ADC) {
    if (ADC = ADC1) {
        return &ADC1_Settings;
    } else {
        return &ADC2_Settings;
    }
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
uint8_t get_adc_msg() {
    return adc_msg;
}

void configure_pll2(void) {
    // turn off PLL2
    if(READ_BIT(RCC->CR, RCC_CR_PLL2RDY)) {
        CLEAR_BIT(RCC->CR, RCC_CR_PLL2ON);
        while(READ_BIT(RCC->CR, RCC_CR_PLL2RDY));
    }

    /*  configure PLL2 (resulting frequency for adc shared bus 50MHz)
        adc own max freq is 25MHz with BOOST = 0b10.
        there is const /2 presc on shared bus, so 50MHz/2 = 25MHz
    */

    // 1. set DIVM2 prescaler (/4)
    MODIFY_REG(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM2, 4U << RCC_PLLCKSELR_DIVM2_Pos);

    // 2. enable pll2p output (adcs)
    if ((READ_BIT(RCC->CR, RCC_CR_PLL2ON) | READ_BIT(RCC->CR, RCC_CR_PLL2RDY))) {
        error = ADC_ERROR_PLL_CONFIG; // critical error
    }
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVP2EN);

    // 3. set pll2 range (after DIVM2 - ref2_ck)
    MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLL2RGE, 0b10 << RCC_PLLCFGR_PLL2RGE_Pos); // 0b10: 4:8MHz

    // 4. set DIVN2 multiplication factor (*75)
    // vco must be 150MHz:420MHz, vco = ref2_ck * DIVN2 = 4MHz * 75 = 300MHz
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL2VCOSEL); // set narrow range for vco
    CLEAR_BIT(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACN2); // no fractions
    MODIFY_REG(RCC->PLL2DIVR, RCC_PLL2DIVR_N2, (75U-1U) << RCC_PLL2DIVR_N2_Pos); // reg 0x03 -> x4, which means set with "-1"

    // 5. set DIVP2 division factor (/6)
    MODIFY_REG(RCC->PLL2DIVR, RCC_PLL2DIVR_P2, (6U-1U) << RCC_PLL2DIVR_P2_Pos); // reg 0x01 -> /2

    /* end configure PLL2 */

    // turn on PLL2
    SET_BIT(RCC->CR, RCC_CR_PLL2ON);

    // turn on buses
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ADC12EN_Msk | RCC_AHB1ENR_DMA1EN);  //Enable ADC 1 and 2
    SET_BIT(RCC->AHB4ENR, RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_ADC3EN); 
}

void adc_init(ADC_TypeDef* ADC, uint8_t* arduino_pins, uint8_t adc_pin_num, ULTRASOUND_DRV_ADC_MODE mode) {

    if (READ_BIT(ADC->CR, ADC_CR_ADCAL | ADC_CR_JADSTART | ADC_CR_ADSTART | ADC_CR_ADSTP | ADC_CR_ADDIS | ADC_CR_ADEN)) {
        error = ADC_ERROR_ADC_CONFIG_VOLTAGE_REGULATOR;
    }

    // exit deep power-down
    CLEAR_BIT(ADC->CR, ADC_CR_DEEPPWD);

    // turn on voltage regulator
    SET_BIT(ADC->CR, ADC_CR_ADVREGEN);
    while(!READ_BIT(ADC->ISR, 0x1UL << 12U)); // LDORDY flag start up time (TADCVREG_STUP)

    // set clock range 12.5MHz:25Mhz
    MODIFY_REG(ADC->CR, ADC_CR_BOOST, 0b10 << ADC_CR_BOOST_Pos);
    
    // set adc clock to async from PLL2 (ADCs must be OFF)
    MODIFY_REG(ADC12_COMMON->CCR, ADC_CCR_CKMODE, 0b00 << ADC_CCR_CKMODE_Pos);
    MODIFY_REG(ADC12_COMMON->CCR, ADC_CCR_PRESC, 0b0000 << ADC_CCR_PRESC_Pos);

    // set overrun mode (overwrite data)
    SET_BIT(ADC->CFGR, ADC_CFGR_OVRMOD);

    // data management
    MODIFY_REG(ADC->CFGR, ADC_CFGR_DMNGT, 0b01 << ADC_CFGR_DMNGT_Pos); // circular DMA mode

    // select channels
    MODIFY_REG(ADC->SQR1, ADC_SQR1_SQ1, (adc_pin_num - 1U) << ADC_SQR1_L_Pos); // how many conversion per seqeunce
    for (uint8_t i = 0; i < adc_pin_num; i++) {
        channel adc_channel = get_adc_channel(arduino_pins[i], ADC);
        SET_BIT(ADC->PCSEL, adc_channel.preselection);
        select_adc_channel(ADC, adc_channel.number, i+1U);

        // sample time (2.5 cycles) + 7.5 cycles (from 16bit res) -> total TCONV = 11 cycles -> 25MHz clock (40ns): 440ns
        set_adc_channel_sample_time(ADC, 0b001, adc_channel.number);
    }

    // if max 500kS/sec, then max 2000ns available for conversion
    // oversampling ratio (x2) -> 440ns * 2 = 880ns per conversion per channel
    MODIFY_REG(ADC->CFGR2, ADC_CFGR2_OVSR, (2U-1U) << ADC_CFGR2_OVSR_Pos); // global for all channels // 2U-1U (temp disable)
    SET_BIT(ADC->CFGR2, ADC_CFGR2_ROVSE);
    MODIFY_REG(ADC->CFGR2, ADC_CFGR2_OVSS, 0b0001 << ADC_CFGR2_OVSS_Pos); // account for x2 oversampling with 1bit right shift for data register
    
    // set operation mode
    switch (mode) {
        case ULTRASOUND_DRV_ADC_MODE_CONT_TIM_TRIGGERED:
            MODIFY_REG(ADC->CFGR, ADC_CFGR_EXTEN, 0b01 << ADC_CFGR_EXTEN_Pos); // enable trigger on rising edge
            MODIFY_REG(ADC->CFGR, ADC_CFGR_EXTSEL, 0b01001 << ADC_CFGR_EXTSEL_Pos); // adc_ext_trg9 from a datasheet (Timer #1)
            CLEAR_BIT(ADC->CFGR, ADC_CFGR_CONT); // set single conversion mode
            break;
        case ULTRASOUND_DRV_ADC_MODE_CONT:
            MODIFY_REG(ADC->CFGR, ADC_CFGR_EXTEN, 0b00 << ADC_CFGR_EXTEN_Pos); // disable hardware trigger
            SET_BIT(ADC->CFGR, ADC_CFGR_CONT); // set continuous mode
            break;
        case ULTRASOUND_DRV_ADC_MODE_ONE_SHOT:
            MODIFY_REG(ADC->CFGR, ADC_CFGR_EXTEN, 0b00 << ADC_CFGR_EXTEN_Pos); // disable hardware trigger
            CLEAR_BIT(ADC->CFGR, ADC_CFGR_CONT); // set single conv mode
            break;
        default:
            error = ADC_ERROR_WRONG_MODE;
            break;
    }

    // calibration
    CLEAR_BIT(ADC->CR, ADC_CR_ADCALDIF); // single ended
    SET_BIT(ADC->CR, ADC_CR_ADCALLIN); // offset and linearity
    SET_BIT(ADC->CR, ADC_CR_ADCAL); // start
    while(READ_BIT(ADC->CR, ADC_CR_ADCAL)); // wait for calibration

    // interrupts
    //SET_BIT(ADC->IER, ADC_IER_EOCIE);
    NVIC_SetPriority(ADC_IRQn, 2);
    NVIC_EnableIRQ(ADC_IRQn);
}

/*
A0 - PC4 - ADC12_INP4
A1 - PC5 - ADC12_INP8
A2 - PB0 - ADC12_INP9
A3 - PB1 - ADC12_INP5
A4 - PC3 - ADC12_INP13
A5 - PC2 - ADC123_INP12
A6 - PC0 - ADC123_INP10
A7 - PA0 - ADC1_INP16
A8 - PC2_C - ADC3_INP0
A9 - PC3_C - ADC3_INP1
A10 - PA1_C - ADC12_INP1
A11 - PA0_C - ADC12_INP0

_С pins could be shorted to their non-_C pins:
CLEAR_BIT(SYSCFG->PMCR, SYSCFG_PMCR_PC3SO);
this shorts PC3_C to PC3
and you could access ADC12_INP13 through PC3_C
*/
channel get_adc_channel(uint8_t arduino_pin, ADC_TypeDef* ADC) {
    channel adc_channel = {.number = 0U, .preselection = 0U};
    switch(arduino_pin) {
        case PIN_A0:
            adc_channel.number = 4U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_4;
            break;
        case PIN_A1:
            adc_channel.number = 8U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_8;
            break;
        case PIN_A2:
            adc_channel.number = 9U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_9;
            break;
        case PIN_A3:
            adc_channel.number = 5U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_5;
            break;
        case PIN_A4:
            adc_channel.number = 13U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_13;
            break;
        case PIN_A5:
            adc_channel.number = 12U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_12;
            break;
        case PIN_A6:
            adc_channel.number = 10U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_10;
            break;
        case PIN_A7:
            if (ADC == ADC1) {
                adc_channel.number = 16U;
                adc_channel.preselection = ADC_PCSEL_PCSEL_16;
            } else {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
            }
            break;
        case A8:
            error = ADC_ERROR_PICKED_WRONG_CHANNEL;
            break;
        case A9:
            error = ADC_ERROR_PICKED_WRONG_CHANNEL;
            break;
        case A10:
            adc_channel.number = 1U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_1;
            break;
        case A11:
            adc_channel.number = 0U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_0;
            break;
        default:
            error = ADC_ERROR_PICKED_WRONG_CHANNEL;
            break;
    }

    return adc_channel;
}

void select_adc_channel(ADC_TypeDef* ADC, uint8_t channel_num, uint8_t rank) {
    switch(rank) {
        case 1:
            MODIFY_REG(ADC->SQR1, ADC_SQR1_SQ1, channel_num << ADC_SQR1_SQ1_Pos);
            break;
        case 2:
            MODIFY_REG(ADC->SQR1, ADC_SQR1_SQ2, channel_num << ADC_SQR1_SQ2_Pos);
            break;
        case 3:
            MODIFY_REG(ADC->SQR1, ADC_SQR1_SQ3, channel_num << ADC_SQR1_SQ3_Pos);
            break;
        case 4:
            MODIFY_REG(ADC->SQR1, ADC_SQR1_SQ4, channel_num << ADC_SQR1_SQ4_Pos);
            break;
        case 5:
            MODIFY_REG(ADC->SQR2, ADC_SQR2_SQ5, channel_num << ADC_SQR2_SQ5_Pos);
            break;
        case 6:
            MODIFY_REG(ADC->SQR2, ADC_SQR2_SQ6, channel_num << ADC_SQR2_SQ6_Pos);
            break;
        case 7:
            MODIFY_REG(ADC->SQR2, ADC_SQR2_SQ7, channel_num << ADC_SQR2_SQ7_Pos);
            break;
        case 8:
            MODIFY_REG(ADC->SQR2, ADC_SQR2_SQ8, channel_num << ADC_SQR2_SQ8_Pos);
            break;
        case 9:
            MODIFY_REG(ADC->SQR2, ADC_SQR2_SQ9, channel_num << ADC_SQR2_SQ9_Pos);
            break;
        case 10:
            MODIFY_REG(ADC->SQR3, ADC_SQR3_SQ10, channel_num << ADC_SQR3_SQ10_Pos);
            break;
        case 11:
            MODIFY_REG(ADC->SQR3, ADC_SQR3_SQ11, channel_num << ADC_SQR3_SQ11_Pos);
            break;
        case 12:
            MODIFY_REG(ADC->SQR3, ADC_SQR3_SQ12, channel_num << ADC_SQR3_SQ12_Pos);
            break;
        case 13:
            MODIFY_REG(ADC->SQR3, ADC_SQR3_SQ13, channel_num << ADC_SQR3_SQ13_Pos);
            break;
        case 14:
            MODIFY_REG(ADC->SQR3, ADC_SQR3_SQ14, channel_num << ADC_SQR3_SQ14_Pos);
            break;
        case 15:
            MODIFY_REG(ADC->SQR4, ADC_SQR4_SQ15, channel_num << ADC_SQR4_SQ15_Pos);
            break;
        case 16:
            MODIFY_REG(ADC->SQR4, ADC_SQR4_SQ16, channel_num << ADC_SQR4_SQ16_Pos);
            break;
        default:
            error = ADC_ERROR_WRONG_SEQUENCE;
            break;
    }
}

void set_adc_channel_sample_time(ADC_TypeDef* ADC, uint8_t sample_time, uint8_t channel_num) {
    switch(channel_num) {
        case 0U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP0, sample_time << ADC_SMPR1_SMP0_Pos);
            break;
        case 1U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP1, sample_time << ADC_SMPR1_SMP1_Pos);
            break;
        case 2U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP2, sample_time << ADC_SMPR1_SMP2_Pos);
            break;
        case 3U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP3, sample_time << ADC_SMPR1_SMP3_Pos);
            break;
        case 4U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP4, sample_time << ADC_SMPR1_SMP4_Pos);
            break;
        case 5U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP5, sample_time << ADC_SMPR1_SMP5_Pos);
            break;
        case 6U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP6, sample_time << ADC_SMPR1_SMP6_Pos);
            break;
        case 7U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP7, sample_time << ADC_SMPR1_SMP7_Pos);
            break;
        case 8U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP8, sample_time << ADC_SMPR1_SMP8_Pos);
            break;
        case 9U:
            MODIFY_REG(ADC->SMPR1, ADC_SMPR1_SMP9, sample_time << ADC_SMPR1_SMP9_Pos);
            break;
        case 10U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP10, sample_time << ADC_SMPR2_SMP10_Pos);
            break;
        case 11U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP11, sample_time << ADC_SMPR2_SMP11_Pos);
            break;
        case 12U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP12, sample_time << ADC_SMPR2_SMP12_Pos);
            break;
        case 13U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP13, sample_time << ADC_SMPR2_SMP13_Pos);
            break;
        case 14U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP14, sample_time << ADC_SMPR2_SMP14_Pos);
            break;
        case 15U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP15, sample_time << ADC_SMPR2_SMP15_Pos);
            break;
        case 16U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP16, sample_time << ADC_SMPR2_SMP16_Pos);
            break;
        case 17U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP17, sample_time << ADC_SMPR2_SMP17_Pos);
            break;
        case 18U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP18, sample_time << ADC_SMPR2_SMP18_Pos);
            break;
        case 19U:
            MODIFY_REG(ADC->SMPR2, ADC_SMPR2_SMP19, sample_time << ADC_SMPR2_SMP19_Pos);
            break;
        default:
            error = ADC_ERROR_SAMPLE_TIME_SETTING;
            break;
    }
}

void ADC_IRQHandler(void) {
    if (READ_BIT(ADC1->ISR, ADC_ISR_EOC)) {
        SET_BIT(ADC1->ISR, ADC_ISR_EOC);
        ADC1_Settings.eoc_flag = 0;
    }
    
    if (READ_BIT(ADC2->ISR, ADC_ISR_EOC)) {
        SET_BIT(ADC2->ISR, ADC_ISR_EOC);
        ADC2_Settings.eoc_flag = 0;
    }
}
