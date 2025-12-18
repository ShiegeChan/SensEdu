#include "adc.h"
#include "timer.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

// ADC masks for identification
#define ADC1_BIT (1U << 0)
#define ADC2_BIT (1U << 1)
#define ADC3_BIT (1U << 2)

// Maximum amount of channels per ADC
#define MAX_CHANNEL_NUM (16U)

// Sequence registers offsets
#define SQR1_REG_ADDR_OFFSET    (0x30)
#define SQR2_REG_ADDR_OFFSET    (0x34)
#define SQR3_REG_ADDR_OFFSET    (0x38)
#define SQR4_REG_ADDR_OFFSET    (0x3C)

// Minimum allowed sampling rate (chosen arbitrary)
static const uint16_t MIN_SAMPLING_RATE = 10;

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */

// Describe ADC channel with the available ADCs, pre-selection mask, and id
typedef struct {
    uint8_t pin_name;       // Arduino pin name
    uint8_t adc_mask;       // ADC mask to determine which ADCs are wired to this channel
    uint8_t pcsel_id;       // Pre-channel selection id
    uint32_t pcsel_mask;    // Pre-channel selection mask
} AdcChannel;

// Describes ADC conversion sequence in multi-channel operation
typedef struct {
    uint16_t reg_offset;    // SQRx register address offset from ADC base
    uint32_t reg_field;     // SQx field mask
    uint8_t reg_field_pos;  // SQx field position
} AdcSequence;

// Describes per ADC runtime state
// Includes DMA/SEQUENCE complete state and soft polling storage
typedef struct {
    volatile bool ovr_flag;                 // Flag notifying that overrun event happened
    volatile uint32_t ovr_counter;          // OVR event counter
    volatile bool dma_complete;             // DMA transfer complete flag
    volatile bool dma_half_transfer;        // DMA half transfer reached flag
    uint16_t seq_buffer[MAX_CHANNEL_NUM];   // software polling data storage
} AdcState;

/* -------------------------------------------------------------------------- */
/*                                    Maps                                    */
/* -------------------------------------------------------------------------- */

static const AdcChannel adc_channel_map[] = {
    {PIN_A0,    ADC1_BIT | ADC2_BIT,            4U,     ADC_PCSEL_PCSEL_4},
    {PIN_A1,    ADC1_BIT | ADC2_BIT,            8U,     ADC_PCSEL_PCSEL_8},
    {PIN_A2,    ADC1_BIT | ADC2_BIT,            9U,     ADC_PCSEL_PCSEL_9},
    {PIN_A3,    ADC1_BIT | ADC2_BIT,            5U,     ADC_PCSEL_PCSEL_5},
    {PIN_A4,    ADC1_BIT | ADC2_BIT,            13U,    ADC_PCSEL_PCSEL_13},
    {PIN_A5,    ADC1_BIT | ADC2_BIT | ADC3_BIT, 12U,    ADC_PCSEL_PCSEL_12},
    {PIN_A6,    ADC1_BIT | ADC2_BIT | ADC3_BIT, 10U,    ADC_PCSEL_PCSEL_10},
    {PIN_A7,    ADC1_BIT,                       16U,    ADC_PCSEL_PCSEL_16},
    {A8,        ADC3_BIT,                       0U,     ADC_PCSEL_PCSEL_0},
    {A9,        ADC3_BIT,                       1U,     ADC_PCSEL_PCSEL_1},
    {A10,       ADC1_BIT | ADC2_BIT,            1U,     ADC_PCSEL_PCSEL_1},
    {A11,       ADC1_BIT | ADC2_BIT,            0U,     ADC_PCSEL_PCSEL_0}
};

static const AdcSequence adc_sequence_map[] = {
    {SQR1_REG_ADDR_OFFSET, ADC_SQR1_SQ1,  ADC_SQR1_SQ1_Pos},
    {SQR1_REG_ADDR_OFFSET, ADC_SQR1_SQ2,  ADC_SQR1_SQ2_Pos},
    {SQR1_REG_ADDR_OFFSET, ADC_SQR1_SQ3,  ADC_SQR1_SQ3_Pos},
    {SQR1_REG_ADDR_OFFSET, ADC_SQR1_SQ4,  ADC_SQR1_SQ4_Pos},
    {SQR2_REG_ADDR_OFFSET, ADC_SQR2_SQ5,  ADC_SQR2_SQ5_Pos},
    {SQR2_REG_ADDR_OFFSET, ADC_SQR2_SQ6,  ADC_SQR2_SQ6_Pos},
    {SQR2_REG_ADDR_OFFSET, ADC_SQR2_SQ7,  ADC_SQR2_SQ7_Pos},
    {SQR2_REG_ADDR_OFFSET, ADC_SQR2_SQ8,  ADC_SQR2_SQ8_Pos},
    {SQR2_REG_ADDR_OFFSET, ADC_SQR2_SQ9,  ADC_SQR2_SQ9_Pos},
    {SQR3_REG_ADDR_OFFSET, ADC_SQR3_SQ10, ADC_SQR3_SQ10_Pos},
    {SQR3_REG_ADDR_OFFSET, ADC_SQR3_SQ11, ADC_SQR3_SQ11_Pos},
    {SQR3_REG_ADDR_OFFSET, ADC_SQR3_SQ12, ADC_SQR3_SQ12_Pos},
    {SQR3_REG_ADDR_OFFSET, ADC_SQR3_SQ13, ADC_SQR3_SQ13_Pos},
    {SQR3_REG_ADDR_OFFSET, ADC_SQR3_SQ14, ADC_SQR3_SQ14_Pos},
    {SQR4_REG_ADDR_OFFSET, ADC_SQR4_SQ15, ADC_SQR4_SQ15_Pos},
    {SQR4_REG_ADDR_OFFSET, ADC_SQR4_SQ16, ADC_SQR4_SQ16_Pos}
};

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

// Global error container
static ADC_ERROR error = ADC_ERROR_NO_ERRORS;

// Per ADC storage containers
static SensEdu_ADC_Settings adc_settings[3];
static AdcState adc_states[3];

// Clock config flag
static bool pll_configured = false;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static SensEdu_ADC_Settings* get_adc_settings(ADC_TypeDef* adc);
static AdcState* get_adc_state(ADC_TypeDef* adc);
static uint8_t get_adc_mask(ADC_TypeDef* adc);
static AdcChannel get_adc_channel(ADC_TypeDef* adc, const uint8_t arduino_pin);
static void select_adc_channel(ADC_TypeDef* adc, uint8_t ch_num, uint8_t conv_num);
static void select_sample_time(ADC_TypeDef* adc, uint8_t ch_num, uint8_t sample_time);
static bool is_dma_mode_enabled(SENSEDU_ADC_MODE adc_mode);
static ADC_ERROR check_settings(SensEdu_ADC_Settings* settings);
static void configure_pll2(void);
static void adc_init(ADC_TypeDef* adc, uint8_t* pins, uint8_t pin_num, SENSEDU_ADC_SR_MODE sr_mode, SENSEDU_ADC_MODE adc_mode);
static uint16_t* read_sequence_cont(ADC_TypeDef* adc, uint8_t pin_num);
static uint16_t* read_sequence_one_shot(ADC_TypeDef* adc, uint8_t pin_num);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

// Initializes ADC with new settings from ADC_Settings struct
// Saves settings individually for each ADC
void SensEdu_ADC_Init(SensEdu_ADC_Settings* new_settings) {

    // Sanity checks
    error = check_settings(new_settings);
    if (error != ADC_ERROR_NO_ERRORS) return;

    // Store settings locally
    SensEdu_ADC_Settings* settings = get_adc_settings(new_settings->adc);
    if (!settings) {
        error = ADC_ERROR_INIT;
        return;
    }
    *settings = *new_settings;

    // Init flags and storage
    AdcState* state = get_adc_state(settings->adc);
    if (!state) {
        error = ADC_ERROR_INIT;
        return;
    }
    state->ovr_flag = false;
    state->ovr_counter = 0;
    state->dma_complete = false;
    state->dma_half_transfer = false;

    if (!pll_configured) {
        configure_pll2();
        pll_configured = true;
    }
    adc_init(settings->adc, settings->pins, settings->pin_num, settings->sr_mode, settings->adc_mode);

    if (settings->sr_mode == SENSEDU_ADC_SR_MODE_FIXED) {
        TIMER_ADCxInit(settings->adc);
        TIMER_ADCxSetFreq(settings->adc, settings->sampling_rate_hz);
    }

    if (is_dma_mode_enabled(settings->adc_mode)) {
        DMA_ADCInit(settings->adc, settings->mem_address, settings->mem_size);
    }
}

// Enables selected ADC
void SensEdu_ADC_Enable(ADC_TypeDef* adc) {
    // Enable timer if in SR fixed mode
    if (get_adc_settings(adc)->sr_mode == SENSEDU_ADC_SR_MODE_FIXED) {
        TIMER_ADCxEnable(adc);
    }

    // Clear ready bit
    SET_BIT(adc->ISR, ADC_ISR_ADRDY);

    // Enable ADC
    SET_BIT(adc->CR, ADC_CR_ADEN);
    while (!READ_BIT(adc->ISR, ADC_ISR_ADRDY)) {}

    // Check if ready to start
    if (!READ_BIT(adc->CR, ADC_CR_ADEN) || READ_BIT(adc->CR, ADC_CR_ADDIS)) {
        error = ADC_ERROR_ENABLE_FAIL;
    }
}

// Disables selected ADC
void SensEdu_ADC_Disable(ADC_TypeDef* adc) {
    // Check if conversion is ongoing
    if (READ_BIT(adc->CR, ADC_CR_ADSTART)) {
        SET_BIT(adc->CR, ADC_CR_ADSTP); // Stop conversion
        while (READ_BIT(adc->CR, ADC_CR_ADSTP)) {} // Wait till it is stopped
    }

    if (READ_BIT(adc->CR, ADC_CR_ADSTART)) {
        error = ADC_ERROR_DISABLE_FAIL;
    }

    SET_BIT(adc->CR, ADC_CR_ADDIS);
    while (READ_BIT(adc->CR, ADC_CR_ADEN)) {}

    if (is_dma_mode_enabled(get_adc_settings(adc)->adc_mode)) {
        DMA_ADCDisable(adc);
    }
}

// Starts selected ADC
// Make sure it is enabled first
void SensEdu_ADC_Start(ADC_TypeDef* adc) {
    if (is_dma_mode_enabled(get_adc_settings(adc)->adc_mode)) {
        DMA_ADCEnable(adc);
    }
    SET_BIT(adc->CR, ADC_CR_ADSTART);
}

// Reads one ADC conversion via software poll
// (not recommended slow alternative to DMA transfers)
uint16_t SensEdu_ADC_ReadConversion(ADC_TypeDef* adc) {
    SensEdu_ADC_Settings* settings = get_adc_settings(adc);
    if (is_dma_mode_enabled(settings->adc_mode)) {
        error = ADC_ERROR_SOFT_POLLING_IN_DMA_MODE;
        return 0;
    }

    if (!READ_BIT(adc->CR, ADC_CR_ADSTART)) {
        error = ADC_ERROR_SOFT_POLLING_ADC_NOT_STARTED;
        return 0;
    }

    if (settings->adc_mode == SENSEDU_ADC_MODE_POLLING_ONE_SHOT) {
        while (!READ_BIT(adc->ISR, ADC_ISR_EOC)) {}
        return READ_REG(adc->DR);
    }

    if (settings->adc_mode == SENSEDU_ADC_MODE_POLLING_CONT) {
        return READ_REG(adc->DR);
    }

    error = ADC_ERROR_UNDEFINED_BEHAVIOUR;
    return 0;
}

// Reads multiple ADC conversions via software poll
// Number of conversions depends on selected amount of channels during initialization
// (not recommended slow alternative to DMA transfers)
uint16_t* SensEdu_ADC_ReadSequence(ADC_TypeDef* adc) {
    SensEdu_ADC_Settings* settings = get_adc_settings(adc);
    if (is_dma_mode_enabled(settings->adc_mode)) {
        error = ADC_ERROR_SOFT_POLLING_IN_DMA_MODE;
        return NULL;
    }

    if (!READ_BIT(adc->CR, ADC_CR_ADSTART)) {
        error = ADC_ERROR_SOFT_POLLING_ADC_NOT_STARTED;
        return NULL;
    }

    if (settings->adc_mode == SENSEDU_ADC_MODE_POLLING_ONE_SHOT) {
        return read_sequence_one_shot(adc, settings->pin_num);
    }

    if (settings->adc_mode == SENSEDU_ADC_MODE_POLLING_CONT) {
        return read_sequence_cont(adc, settings->pin_num);
    }

    error = ADC_ERROR_UNDEFINED_BEHAVIOUR;
    return NULL;
}

// Enables overrun interrupts which allows SensEdu_ADC_GetOverrunState
// and SensEdu_ADC_GetOverrunCounter to show the amount of missing samples
//
// Useful for software poll frequency tests
//
// Be careful: in SENSEDU_ADC_SR_MODE_FREE this can cause an interrupt storm
void SensEdu_ADC_EnableOverrunInterrupt(ADC_TypeDef* adc) {
    SET_BIT(adc->IER, ADC_IER_OVRIE);
}

// Disables overrun interrupts
void SensEdu_ADC_DisableOverrunInterrupt(ADC_TypeDef* adc) {
    CLEAR_BIT(adc->IER, ADC_IER_OVRIE);
}

// Outputs a global overrun flag showing if the event ever happened
bool SensEdu_ADC_IsOverrun(ADC_TypeDef* adc) {
    return get_adc_state(adc)->ovr_flag;
}

// Clears a global overrun flag
void SensEdu_ADC_ClearOverrun(ADC_TypeDef* adc) {
    get_adc_state(adc)->ovr_flag = false;
}

// Outputs a counter with the number of times overrun event happened
uint32_t SensEdu_ADC_GetOverrunCount(ADC_TypeDef* adc) {
    return get_adc_state(adc)->ovr_counter;
}

// Outputs DMA transfer completion status flag
bool SensEdu_ADC_IsDmaTransferComplete(ADC_TypeDef *adc) {
    return get_adc_state(adc)->dma_complete;
}

// Clears DMA transfer completion status flag
void SensEdu_ADC_ClearDmaTransferComplete(ADC_TypeDef* adc) {
    get_adc_state(adc)->dma_complete = false;
}

// Outputs DMA half transfer reached status flag
bool SensEdu_ADC_IsDmaHalfTransferComplete(ADC_TypeDef *adc) {
    return get_adc_state(adc)->dma_half_transfer;
}

// Clears DMA half transfer reached status flag
void SensEdu_ADC_ClearDmaHalfTransferComplete(ADC_TypeDef* adc) {
    get_adc_state(adc)->dma_half_transfer = false;
}

// Outputs ADC error code
ADC_ERROR ADC_GetError(void) {
    return error;
}

// Sets DMA transfer completion status flag
void ADC_SetDmaTransferComplete(ADC_TypeDef* adc) {
    get_adc_state(adc)->dma_complete = true;
}

// Sets DMA half transfer reached status flag
void ADC_SetDmaHalfTransferComplete(ADC_TypeDef* adc) {
    get_adc_state(adc)->dma_half_transfer = true;
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */

static SensEdu_ADC_Settings* get_adc_settings(ADC_TypeDef* adc) {
    if (adc == ADC1) return &adc_settings[0];
    if (adc == ADC2) return &adc_settings[1];
    if (adc == ADC3) return &adc_settings[2];
    error = ADC_ERROR_WRONG_ADC_INSTANCE;
    return NULL;
}

static AdcState* get_adc_state(ADC_TypeDef* adc) {
    if (adc == ADC1) return &adc_states[0];
    if (adc == ADC2) return &adc_states[1];
    if (adc == ADC3) return &adc_states[2];
    error = ADC_ERROR_WRONG_ADC_INSTANCE;
    return NULL;
}

static uint8_t get_adc_mask(ADC_TypeDef* adc) {
    if (adc == ADC1) return ADC1_BIT;
    if (adc == ADC2) return ADC2_BIT;
    if (adc == ADC3) return ADC3_BIT;
    return 0U;
}

// Outputs AdcChannel structure containing all needed masks, ids etc. needed for channel configuration
// based on selected ADC and arduino pin
static AdcChannel get_adc_channel(ADC_TypeDef* adc, const uint8_t arduino_pin) {
    for (size_t i = 0; i < ARRAY_SIZE(adc_channel_map); i++) {
        if (adc_channel_map[i].pin_name != arduino_pin) {
            continue;
        }
        if ((adc_channel_map[i].adc_mask & get_adc_mask(adc)) == 0U) {
            error = ADC_ERROR_PICKED_WRONG_CHANNEL;
            break;
        }
        return adc_channel_map[i];
    }
    error = ADC_ERROR_PICKED_WRONG_CHANNEL;
    return (AdcChannel){0};
}

// Configures selected ADC channel in a conversion sequence
//
// ch_num: ADC pre-channel selection id
// conv_num: Position in a sequence (1st, 2nd, ... conversion in a sequence)
//
// Chapter 26.6.11 ADC regular sequence register
static void select_adc_channel(ADC_TypeDef* adc, uint8_t ch_num, uint8_t conv_num) {
    if (conv_num == 0U || conv_num > ARRAY_SIZE(adc_sequence_map) || ch_num > 0b11111) {
        error = ADC_ERROR_CHANNEL_SETTING;
        return;
    }
    const AdcSequence* seq = &adc_sequence_map[conv_num - 1];
    volatile uint32_t* reg = (volatile uint32_t*)((uint8_t*)adc + seq->reg_offset);
    MODIFY_REG(*reg, seq->reg_field, ch_num << seq->reg_field_pos);
}

// Configures sampling time for selected ADC channel
//
// Chapter 26.6.6 ADC sample time register
static void select_sample_time(ADC_TypeDef* adc, uint8_t ch_num, uint8_t sample_time) {
    if (ch_num > 19U || sample_time > 0x7U) {
        error = ADC_ERROR_SAMPLE_TIME_SETTING;
        return;
    }
    if (ch_num < 10U) {
        uint8_t smpx_pos = 3 * ch_num;
        MODIFY_REG(adc->SMPR1, 0x7UL << smpx_pos, sample_time << smpx_pos);
    } else {
        uint8_t smpx_pos = 3 * (ch_num - 10);
        MODIFY_REG(adc->SMPR2, 0x7UL << smpx_pos, sample_time << smpx_pos);
    }
}

static bool is_dma_mode_enabled(SENSEDU_ADC_MODE adc_mode) {
    return (adc_mode == SENSEDU_ADC_MODE_DMA_NORMAL || adc_mode == SENSEDU_ADC_MODE_DMA_CIRCULAR);
}

// Sanity checks
static ADC_ERROR check_settings(SensEdu_ADC_Settings* settings) {
    if (!settings) {
        return ADC_ERROR_INIT;
    }

    if (settings->adc != ADC1 && settings->adc != ADC2 && settings->adc != ADC3) {
        return ADC_ERROR_WRONG_ADC_INSTANCE;
    }

    if (settings->pin_num < 1 || settings->pin_num > MAX_CHANNEL_NUM) {
        return ADC_ERROR_INIT_PIN_NUMBER;
    }

    if (settings->pins == NULL) {
        return ADC_ERROR_INIT_PIN_ARRAY;
    }

    if (settings->sr_mode == SENSEDU_ADC_SR_MODE_FIXED && settings->sampling_rate_hz < MIN_SAMPLING_RATE) {
        return ADC_ERROR_INIT_SAMPLING_RATE;
    }

    if (is_dma_mode_enabled(settings->adc_mode)) {
        if (settings->mem_address == 0x0000 || settings->mem_address == NULL || settings->mem_size == 0) {
            return ADC_ERROR_INIT_DMA;
        }
    }

    if (settings->adc_mode == SENSEDU_ADC_MODE_POLLING_ONE_SHOT && settings->sr_mode == SENSEDU_ADC_SR_MODE_FIXED) {
        return ADC_ERROR_INIT_ONE_SHOT_SR;
    }

    return ADC_ERROR_NO_ERRORS;
}

// Configures main ADC clock on PLL2
static void configure_pll2(void) {
    // turn off PLL2
    if (READ_BIT(RCC->CR, RCC_CR_PLL2ON)) {
        CLEAR_BIT(RCC->CR, RCC_CR_PLL2ON);
        while (READ_BIT(RCC->CR, RCC_CR_PLL2RDY)) {}
    }

    /*  configure PLL2 (resulting frequency for adc shared bus 50MHz)
        adc own max freq is 25MHz with BOOST = 0b10.
        there is const /2 presc on shared bus, so 50MHz/2 = 25MHz
    */

    // 1. set DIVM2 prescaler (/4)
    MODIFY_REG(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM2, 4U << RCC_PLLCKSELR_DIVM2_Pos);

    // 2. enable pll2p output (adcs)
    if (READ_BIT(RCC->CR, RCC_CR_PLL2ON | RCC_CR_PLL2RDY)) {
        error = ADC_ERROR_PLL_CONFIG; // critical error
        return;
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
    while (!READ_BIT(RCC->CR, RCC_CR_PLL2RDY)) {}

    // turn on buses
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ADC12EN_Msk | RCC_AHB1ENR_DMA1EN);
    SET_BIT(RCC->AHB4ENR, RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_ADC3EN);

    // set adc clock to async from PLL2 (ADCs must be OFF)
    MODIFY_REG(ADC12_COMMON->CCR, ADC_CCR_CKMODE, 0b00 << ADC_CCR_CKMODE_Pos);
    MODIFY_REG(ADC12_COMMON->CCR, ADC_CCR_PRESC, 0b0000 << ADC_CCR_PRESC_Pos);
    MODIFY_REG(ADC3_COMMON->CCR, ADC_CCR_CKMODE, 0b00 << ADC_CCR_CKMODE_Pos);
    MODIFY_REG(ADC3_COMMON->CCR, ADC_CCR_PRESC, 0b0000 << ADC_CCR_PRESC_Pos);
}

// Initializes selected ADC
static void adc_init(ADC_TypeDef* adc, uint8_t* pins, uint8_t pin_num, SENSEDU_ADC_SR_MODE sr_mode, SENSEDU_ADC_MODE adc_mode) {

    if (READ_BIT(adc->CR, ADC_CR_ADCAL | ADC_CR_JADSTART | ADC_CR_ADSTART | ADC_CR_ADSTP | ADC_CR_ADDIS | ADC_CR_ADEN)) {
        error = ADC_ERROR_INIT;
        return;
    }

    // exit deep power-down
    CLEAR_BIT(adc->CR, ADC_CR_DEEPPWD);

    // turn on voltage regulator
    SET_BIT(adc->CR, ADC_CR_ADVREGEN);
    while (!READ_BIT(adc->ISR, 0x1UL << 12U)) {} // LDORDY flag start up time (TADCVREG_STUP)

    // set clock range 12.5MHz:25Mhz
    MODIFY_REG(adc->CR, ADC_CR_BOOST, 0b10 << ADC_CR_BOOST_Pos);

    // data management
    switch (adc_mode) {
        case SENSEDU_ADC_MODE_POLLING_ONE_SHOT:
            CLEAR_BIT(adc->CFGR, ADC_CFGR_OVRMOD); // do not overwrite data
            MODIFY_REG(adc->CFGR, ADC_CFGR_DMNGT, 0b00 << ADC_CFGR_DMNGT_Pos); // data stored in DR only
            break;
        case SENSEDU_ADC_MODE_POLLING_CONT:
            SET_BIT(adc->CFGR, ADC_CFGR_OVRMOD); // overwrite data
            MODIFY_REG(adc->CFGR, ADC_CFGR_DMNGT, 0b00 << ADC_CFGR_DMNGT_Pos); // data stored in DR only
            break;
        case SENSEDU_ADC_MODE_DMA_NORMAL:
            SET_BIT(adc->CFGR, ADC_CFGR_OVRMOD); // overwrite data
            MODIFY_REG(adc->CFGR, ADC_CFGR_DMNGT, 0b01 << ADC_CFGR_DMNGT_Pos); // DMA one shot mode
            break;
        case SENSEDU_ADC_MODE_DMA_CIRCULAR:
            SET_BIT(adc->CFGR, ADC_CFGR_OVRMOD); // overwrite data
            MODIFY_REG(adc->CFGR, ADC_CFGR_DMNGT, 0b11 << ADC_CFGR_DMNGT_Pos); // DMA circular mode
            break;
        default:
            error = ADC_ERROR_WRONG_OPERATION_MODE;
            break;
    }

    // select channels
    MODIFY_REG(adc->SQR1, ADC_SQR1_L, (pin_num - 1U) << ADC_SQR1_L_Pos); // how many conversions per sequence
    for (uint8_t i = 0; i < pin_num; i++) {
        AdcChannel ch = get_adc_channel(adc, pins[i]);
        SET_BIT(adc->PCSEL, ch.pcsel_mask);
        select_adc_channel(adc, ch.pcsel_id, i + 1);

        // sample time (2.5 cycles) + 7.5 cycles (from 16bit res) -> total TCONV = 11 cycles -> 25MHz clock (40ns): 440ns
        select_sample_time(adc, ch.pcsel_id, 0b001);
    }

    // if max 500kS/sec, then max 2000ns available for conversion
    // oversampling ratio (x2) -> 440ns * 2 = 880ns per conversion per channel
    MODIFY_REG(adc->CFGR2, ADC_CFGR2_OVSR, (2U-1U) << ADC_CFGR2_OVSR_Pos); // global for all channels (x2)
    SET_BIT(adc->CFGR2, ADC_CFGR2_ROVSE);
    MODIFY_REG(adc->CFGR2, ADC_CFGR2_OVSS, 0b0001 << ADC_CFGR2_OVSS_Pos); // account for x2 with 1-bit right shift

    // set operation mode
    // TODO: verify adc_ext_trg9 and why it is named Timer #1
    switch (sr_mode) {
        case SENSEDU_ADC_SR_MODE_FIXED:
            MODIFY_REG(adc->CFGR, ADC_CFGR_EXTEN, 0b01 << ADC_CFGR_EXTEN_Pos); // enable trigger on rising edge
            MODIFY_REG(adc->CFGR, ADC_CFGR_EXTSEL, 0b01001 << ADC_CFGR_EXTSEL_Pos); // adc_ext_trg9 from a datasheet (Timer #1)
            CLEAR_BIT(adc->CFGR, ADC_CFGR_CONT); // single conversion mode
            break;
        case SENSEDU_ADC_SR_MODE_FREE:
            MODIFY_REG(adc->CFGR, ADC_CFGR_EXTEN, 0b00 << ADC_CFGR_EXTEN_Pos); // disable hardware trigger
            SET_BIT(adc->CFGR, ADC_CFGR_CONT); // continuous mode
            if (adc_mode == SENSEDU_ADC_MODE_POLLING_ONE_SHOT) {
                CLEAR_BIT(adc->CFGR, ADC_CFGR_CONT); // single conversion mode
            }
            break;
        default:
            error = ADC_ERROR_WRONG_OPERATION_MODE;
            break;
    }

    // calibration
    CLEAR_BIT(adc->CR, ADC_CR_ADCALDIF); // single ended
    SET_BIT(adc->CR, ADC_CR_ADCALLIN); // offset and linearity
    SET_BIT(adc->CR, ADC_CR_ADCAL); // start
    while (READ_BIT(adc->CR, ADC_CR_ADCAL)) {} // wait for calibration

    // enable interrupts for software polling (ovr flag)
    if (!is_dma_mode_enabled(adc_mode)) {
        if (adc == ADC1 || adc == ADC2) {
            NVIC_SetPriority(ADC_IRQn, 2);
            NVIC_EnableIRQ(ADC_IRQn);
        }
        if (adc == ADC3) {
            NVIC_SetPriority(ADC3_IRQn, 2);
            NVIC_EnableIRQ(ADC3_IRQn);
        }
    }
}

static uint16_t* read_sequence_cont(ADC_TypeDef* adc, uint8_t pin_num) {
    AdcState* data = get_adc_state(adc);

    // Synchronize to the start of the next sequence
    while (!READ_BIT(adc->ISR, ADC_ISR_EOS)) {}
    SET_BIT(adc->ISR, ADC_ISR_EOS);

    // Poll each channel in order
    for (size_t i = 0; i < pin_num; i++) {
        while (!READ_BIT(adc->ISR, ADC_ISR_EOC)) {}
        data->seq_buffer[i] = READ_REG(adc->DR);
    }
    return data->seq_buffer;
}

static uint16_t* read_sequence_one_shot(ADC_TypeDef* adc, uint8_t pin_num) {
    AdcState* adc_state = get_adc_state(adc);
    for (size_t i = 0; i < pin_num; i++) {
        while (!READ_BIT(adc->ISR, ADC_ISR_EOC)) {}
        adc_state->seq_buffer[i] = READ_REG(adc->DR);
    }
    return adc_state->seq_buffer;
}

/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */

void ADC_IRQHandler(void) {
    if (READ_BIT(ADC1->ISR, ADC_ISR_OVR)) {
        adc_states[0].ovr_flag = true;
        if (adc_states[0].ovr_counter < UINT32_MAX) {
            adc_states[0].ovr_counter++;
        }
        SET_BIT(ADC1->ISR, ADC_ISR_OVR);
    }

    if (READ_BIT(ADC2->ISR, ADC_ISR_OVR)) {
        adc_states[1].ovr_flag = true;
        if (adc_states[1].ovr_counter < UINT32_MAX) {
            adc_states[1].ovr_counter++;
        }
        SET_BIT(ADC2->ISR, ADC_ISR_OVR);
    }
}

void ADC3_IRQHandler(void) {
    if (READ_BIT(ADC3->ISR, ADC_ISR_OVR)) {
        adc_states[2].ovr_flag = true;
        if (adc_states[2].ovr_counter < UINT32_MAX) {
            adc_states[2].ovr_counter++;
        }
        SET_BIT(ADC3->ISR, ADC_ISR_OVR);
    }
}
