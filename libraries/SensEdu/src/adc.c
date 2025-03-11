#include "adc.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */
typedef struct {
    uint8_t number;
    uint32_t preselection;
} channel;

typedef struct {
    volatile uint8_t dma_complete;
    volatile uint8_t eoc_flag;  // End of Conversion flag
    volatile uint8_t eoc_cntr;  // EOC counter for multi-channel sequence
    uint16_t sequence_data[16]; // software polling data storage
} adc_data;


/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static ADC_ERROR error = ADC_ERROR_NO_ERRORS;

static SensEdu_ADC_Settings ADC1_Settings = {ADC1, 0, 0, SENSEDU_ADC_MODE_ONE_SHOT, 0, SENSEDU_ADC_DMA_DISCONNECT, 0, 0};
static SensEdu_ADC_Settings ADC2_Settings = {ADC2, 0, 0, SENSEDU_ADC_MODE_ONE_SHOT, 0, SENSEDU_ADC_DMA_DISCONNECT, 0, 0};
static SensEdu_ADC_Settings ADC3_Settings = {ADC3, 0, 0, SENSEDU_ADC_MODE_ONE_SHOT, 0, SENSEDU_ADC_DMA_DISCONNECT, 0, 0};

static adc_data adc1_data;
static adc_data adc2_data;
static adc_data adc3_data;

static uint16_t pll_configured = 0;


/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
SensEdu_ADC_Settings* get_adc_settings(ADC_TypeDef* ADC);
adc_data* get_adc_data(ADC_TypeDef* ADC);
static ADC_ERROR check_settings(SensEdu_ADC_Settings* settings);
void configure_pll2(void);
void adc_init(ADC_TypeDef* ADC, uint8_t* arduino_pins, uint8_t adc_pin_num, SENSEDU_ADC_CONVMODE mode, SENSEDU_ADC_DMA adc_dma);
channel get_adc_channel(uint8_t arduino_pin, ADC_TypeDef* ADC);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
void SensEdu_ADC_Init(SensEdu_ADC_Settings* adc_settings) {

    // Sanity checks
    error = check_settings(adc_settings);

    // Store settings locally
    SensEdu_ADC_Settings* settings = get_adc_settings(adc_settings->adc);
    *settings = *adc_settings;

    // Init flags and storage
    get_adc_data(settings->adc)->eoc_flag = 0;
    get_adc_data(settings->adc)->eoc_cntr = 0;
    get_adc_data(settings->adc)->dma_complete = 0;


    // Init TIMER, Clock and ADC
    TIMER_ADC1Init();
    if (!pll_configured) {
        configure_pll2();
    }
    adc_init(settings->adc, settings->pins, settings->pin_num, 
        settings->conv_mode, settings->dma_mode);

    // timer setttings if in timer triggered mode
    if (settings->conv_mode == SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED) {
        TIMER_ADC1SetFreq(settings->sampling_freq);
    }

    // dma settings if in dma mode
    if (settings->dma_mode == SENSEDU_ADC_DMA_CONNECT) {
        DMA_ADCInit(settings->adc, settings->mem_address, settings->mem_size);
    }
}

void SensEdu_ADC_Enable(ADC_TypeDef* ADC) {
    // enable timer if in timer triggered mode
    if (get_adc_settings(ADC)->conv_mode == SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED) {
        TIMER_ADC1Enable();
    }

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

void SensEdu_ADC_Disable(ADC_TypeDef* ADC) {
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

    // disable DMA
    if (get_adc_settings(ADC)->dma_mode == SENSEDU_ADC_DMA_CONNECT) {
        DMA_ADCDisable(ADC);
    }
}

void SensEdu_ADC_Start(ADC_TypeDef* ADC) {
    // enable DMA
    if (get_adc_settings(ADC)->dma_mode == SENSEDU_ADC_DMA_CONNECT) {
        DMA_ADCEnable(ADC);
    }

    // start conversions
    SET_BIT(ADC->CR, ADC_CR_ADSTART);
}

// Software Polling (slow alternative to DMA transfers)
// Multi-Channel
uint16_t* SensEdu_ADC_ReadSequence(ADC_TypeDef* ADC) {
    SensEdu_ADC_Settings* settings = get_adc_settings(ADC);
    adc_data* data = get_adc_data(ADC);

    if (settings->conv_mode == SENSEDU_ADC_MODE_ONE_SHOT) {
        error = ADC_ERROR_NOT_SUPPORTED_MODE; // currently broken for this mode
        return;
    }

    // software polled sequences in cont mode are not stable due to synchronization issues
    // adc reset helps
    if (READ_BIT(ADC->CR, ADC_CR_ADSTART)) {
        SET_BIT(ADC->CR, ADC_CR_ADSTP);
        while(READ_BIT(ADC->CR, ADC_CR_ADSTP));
    }

    // set interrupt parameters before start
    data->eoc_flag = 1;
    data->eoc_cntr = settings->pin_num;

    SET_BIT(ADC->CR, ADC_CR_ADSTART);
    while (data->eoc_flag);

    return data->sequence_data;
}

// Software Polling (slow alternative to DMA transfers)
// Single-Channel
uint16_t SensEdu_ADC_ReadConversion(ADC_TypeDef* ADC) {
    return READ_REG(ADC->DR);
}

uint8_t SensEdu_ADC_GetTransferStatus(ADC_TypeDef* adc) {
    return get_adc_data(adc)->dma_complete;
}

void SensEdu_ADC_ClearTransferStatus(ADC_TypeDef* adc) {
    get_adc_data(adc)->dma_complete = 0;
}

// Early versions of the board are using 
// A9 - PC3_C - ADC3_INP1
// as microphone input
//
// if you want to use ADC1 or ADC2 for this microphone,
// you could short PC3 (A4) to PC3_C (A9)
void SensEdu_ADC_ShortA4toA9(void) {
    CLEAR_BIT(SYSCFG->PMCR, SYSCFG_PMCR_PC3SO);
}

ADC_ERROR ADC_GetError(void) {
    return error;
}

void ADC_TransferCompleteDMAinterrupt(ADC_TypeDef* adc) {
    get_adc_data(adc)->dma_complete = 1;
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
SensEdu_ADC_Settings* get_adc_settings(ADC_TypeDef* ADC) {
    if (ADC == ADC1) {
        return &ADC1_Settings;
    } else if (ADC == ADC2) {
        return &ADC2_Settings;
    } else if (ADC == ADC3) {
        return &ADC3_Settings;
    }
}

adc_data* get_adc_data(ADC_TypeDef* ADC) {
    if (ADC == ADC1) {
        return &adc1_data;
    } else if (ADC == ADC2) {
        return &adc2_data;
    } else if (ADC == ADC3) {
        return &adc3_data;
    }
}

static ADC_ERROR check_settings(SensEdu_ADC_Settings* settings) {
    if (settings->adc != ADC1 && settings->adc != ADC2 && settings->adc != ADC3) {
        return ADC_ERROR_WRONG_ADC_INSTANCE;
    } 

    if (settings->pin_num < 1) {
        return ADC_ERROR_INIT_PIN_NUMBER;
    } 

    if (settings->dma_mode == SENSEDU_ADC_DMA_CONNECT) {
        if (settings->mem_address == 0x0000 || settings->mem_size == 0) {
            return ADC_ERROR_INIT_DMA_MEMORY;
        }
    }

    if (settings->conv_mode == SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED) {
        if (settings->sampling_freq < 1000) {
            return ADC_ERROR_INIT_SAMPLING_FREQ;
        }
    } 

    return ADC_ERROR_NO_ERRORS;
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
    MODIFY_REG(RCC->PLL2DIVR, RCC_PLL2DIVR_N2, (75U-1U) << RCC_PLL2DIVR_N2_Pos); // e.g., reg 0x03 -> x4, which means set with "-1"

    // 5. set DIVP2 division factor (/6)
    MODIFY_REG(RCC->PLL2DIVR, RCC_PLL2DIVR_P2, (6U-1U) << RCC_PLL2DIVR_P2_Pos); // e.g., reg 0x01 -> /2

    /* end configure PLL2 */

    // turn on PLL2
    SET_BIT(RCC->CR, RCC_CR_PLL2ON);
    while (!READ_BIT(RCC->CR, RCC_CR_PLL2RDY));

    // turn on buses
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ADC12EN_Msk | RCC_AHB1ENR_DMA1EN);
    SET_BIT(RCC->AHB4ENR, RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_ADC3EN); 

    // set adc clock to async from PLL2 (ADCs must be OFF)
    MODIFY_REG(ADC12_COMMON->CCR, ADC_CCR_CKMODE, 0b00 << ADC_CCR_CKMODE_Pos);
    MODIFY_REG(ADC12_COMMON->CCR, ADC_CCR_PRESC, 0b0000 << ADC_CCR_PRESC_Pos);
    MODIFY_REG(ADC3_COMMON->CCR, ADC_CCR_CKMODE, 0b00 << ADC_CCR_CKMODE_Pos);
    MODIFY_REG(ADC3_COMMON->CCR, ADC_CCR_PRESC, 0b0000 << ADC_CCR_PRESC_Pos);

    // flag
    pll_configured = 1;
}

void adc_init(ADC_TypeDef* ADC, uint8_t* arduino_pins, uint8_t adc_pin_num, SENSEDU_ADC_CONVMODE mode, SENSEDU_ADC_DMA adc_dma) {

    if (READ_BIT(ADC->CR, ADC_CR_ADCAL | ADC_CR_JADSTART | ADC_CR_ADSTART | ADC_CR_ADSTP | ADC_CR_ADDIS | ADC_CR_ADEN)) {
        error = ADC_ERROR_ADC_INIT;
    }

    // exit deep power-down
    CLEAR_BIT(ADC->CR, ADC_CR_DEEPPWD);

    // turn on voltage regulator
    SET_BIT(ADC->CR, ADC_CR_ADVREGEN);
    while(!READ_BIT(ADC->ISR, 0x1UL << 12U)); // LDORDY flag start up time (TADCVREG_STUP)

    // set clock range 12.5MHz:25Mhz
    MODIFY_REG(ADC->CR, ADC_CR_BOOST, 0b10 << ADC_CR_BOOST_Pos);

    // overrun mode (overwrite data)
    SET_BIT(ADC->CFGR, ADC_CFGR_OVRMOD); 

    // data management
    if (adc_dma == SENSEDU_ADC_DMA_CONNECT) {
        MODIFY_REG(ADC->CFGR, ADC_CFGR_DMNGT, 0b01 << ADC_CFGR_DMNGT_Pos); // circular DMA mode
    } else if (adc_dma == SENSEDU_ADC_DMA_DISCONNECT) {
        MODIFY_REG(ADC->CFGR, ADC_CFGR_DMNGT, 0b00 << ADC_CFGR_DMNGT_Pos); // data stored in DR only
    } else {
        error = ADC_ERROR_WRONG_DATA_MANAGEMENT_MODE;
    }
    
    // select channels
    MODIFY_REG(ADC->SQR1, ADC_SQR1_L, (adc_pin_num - 1U) << ADC_SQR1_L_Pos); // how many conversion per seqeunce
    for (uint8_t i = 0; i < adc_pin_num; i++) {
        channel adc_channel = get_adc_channel(arduino_pins[i], ADC);
        SET_BIT(ADC->PCSEL, adc_channel.preselection);
        select_adc_channel(ADC, adc_channel.number, i+1U);

        // sample time (2.5 cycles) + 7.5 cycles (from 16bit res) -> total TCONV = 11 cycles -> 25MHz clock (40ns): 440ns
        set_adc_channel_sample_time(ADC, 0b001, adc_channel.number);
    }

    // if max 500kS/sec, then max 2000ns available for conversion
    // oversampling ratio (x2) -> 440ns * 2 = 880ns per conversion per channel
    MODIFY_REG(ADC->CFGR2, ADC_CFGR2_OVSR, (2U-1U) << ADC_CFGR2_OVSR_Pos); // global for all channels
    SET_BIT(ADC->CFGR2, ADC_CFGR2_ROVSE);
    MODIFY_REG(ADC->CFGR2, ADC_CFGR2_OVSS, 0b0001 << ADC_CFGR2_OVSS_Pos); // account for x2 oversampling with 1bit right shift for data register
    
    // set operation mode
    switch (mode) {
        case SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED:
            MODIFY_REG(ADC->CFGR, ADC_CFGR_EXTEN, 0b01 << ADC_CFGR_EXTEN_Pos); // enable trigger on rising edge
            MODIFY_REG(ADC->CFGR, ADC_CFGR_EXTSEL, 0b01001 << ADC_CFGR_EXTSEL_Pos); // adc_ext_trg9 from a datasheet (Timer #1)
            CLEAR_BIT(ADC->CFGR, ADC_CFGR_CONT); // set single conversion mode
            break;
        case SENSEDU_ADC_MODE_CONT:
            MODIFY_REG(ADC->CFGR, ADC_CFGR_EXTEN, 0b00 << ADC_CFGR_EXTEN_Pos); // disable hardware trigger
            SET_BIT(ADC->CFGR, ADC_CFGR_CONT); // set continuous mode
            break;
        case SENSEDU_ADC_MODE_ONE_SHOT:
            MODIFY_REG(ADC->CFGR, ADC_CFGR_EXTEN, 0b00 << ADC_CFGR_EXTEN_Pos); // disable hardware trigger
            CLEAR_BIT(ADC->CFGR, ADC_CFGR_CONT); // set single conv mode
            break;
        default:
            error = ADC_ERROR_WRONG_OPERATION_MODE;
            break;
    }

    // calibration
    CLEAR_BIT(ADC->CR, ADC_CR_ADCALDIF); // single ended
    SET_BIT(ADC->CR, ADC_CR_ADCALLIN); // offset and linearity
    SET_BIT(ADC->CR, ADC_CR_ADCAL); // start
    while(READ_BIT(ADC->CR, ADC_CR_ADCAL)); // wait for calibration

    // interrupts (only for software polling)
    if (adc_dma == SENSEDU_ADC_DMA_DISCONNECT) {
        SET_BIT(ADC->IER, ADC_IER_EOCIE);
        if (ADC == ADC1 || ADC == ADC2) {
            NVIC_SetPriority(ADC_IRQn, 2);
            NVIC_EnableIRQ(ADC_IRQn);
        } else if (ADC == ADC3) {
            NVIC_SetPriority(ADC3_IRQn, 2);
            NVIC_EnableIRQ(ADC3_IRQn);
        }
    }
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
            if (ADC == ADC3) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
            adc_channel.number = 4U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_4;
            break;
        case PIN_A1:
            if (ADC == ADC3) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
            adc_channel.number = 8U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_8;
            break;
        case PIN_A2:
            if (ADC == ADC3) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
            adc_channel.number = 9U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_9;
            break;
        case PIN_A3:
            if (ADC == ADC3) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
            adc_channel.number = 5U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_5;
            break;
        case PIN_A4:
            if (ADC == ADC3) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
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
            if (ADC == ADC2 || ADC == ADC3) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
            adc_channel.number = 16U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_16;
            break;
        case A8:
            if (ADC == ADC1 || ADC == ADC2) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
            adc_channel.number = 0U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_0;
            break;
        case A9:
            if (ADC == ADC1 || ADC == ADC2) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
            adc_channel.number = 1U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_1;
            break;
        case A10:
            if (ADC == ADC3) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
            adc_channel.number = 1U;
            adc_channel.preselection = ADC_PCSEL_PCSEL_1;
            break;
        case A11:
            if (ADC == ADC3) {
                error = ADC_ERROR_PICKED_WRONG_CHANNEL;
                break;
            }
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


/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */
void ADC_IRQHandler(void) {
    if (READ_BIT(ADC1->ISR, ADC_ISR_EOC)) {
        SET_BIT(ADC1->ISR, ADC_ISR_EOC);
        if (adc1_data.eoc_flag) {
            adc1_data.sequence_data[ADC1_Settings.pin_num - adc1_data.eoc_cntr] = READ_REG(ADC1->DR);
            adc1_data.eoc_cntr = adc1_data.eoc_cntr - 1;
            if (adc1_data.eoc_cntr == 0) {
                adc1_data.eoc_flag = 0;
            }
        }
    }
    
    if (READ_BIT(ADC2->ISR, ADC_ISR_EOC)) {
        SET_BIT(ADC2->ISR, ADC_ISR_EOC);
        if (adc2_data.eoc_flag) {
            adc2_data.sequence_data[ADC2_Settings.pin_num - adc2_data.eoc_cntr] = READ_REG(ADC2->DR);
            adc2_data.eoc_cntr = adc2_data.eoc_cntr - 1;
            if (adc2_data.eoc_cntr == 0) {
                adc2_data.eoc_flag = 0;
            }
        }
    }
}

void ADC3_IRQHandler(void) {
    if (READ_BIT(ADC3->ISR, ADC_ISR_EOC)) {
        SET_BIT(ADC3->ISR, ADC_ISR_EOC);
        if (adc3_data.eoc_flag) {
            adc3_data.sequence_data[ADC3_Settings.pin_num - adc3_data.eoc_cntr] = READ_REG(ADC3->DR);
            adc3_data.eoc_cntr = adc3_data.eoc_cntr - 1;
            if (adc3_data.eoc_cntr == 0) {
                adc3_data.eoc_flag = 0;
            }
        }
    }
}
