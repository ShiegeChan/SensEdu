/**
 * @file dac.c
 * @brief Internal implementation of DAC driver (DMA-based waveform generation).
 *
 * This module provides:
 * - DAC waveform generation using DMA
 * - Timer-triggered DAC sampling
 * - Burst and continuous waveform modes
 *
 * User constraints:
 * - SensEdu_DAC_Init() must be called before Enable()
 * - Both channels must use identical sampling_freq
 *
 * Notes:
 * - DAC_Channel is an opaque identifier implemented as a pointer with DAC_CH1 and DAC_CH2 only valid values
 * - DAC output is driven by DMA + Timer
 * - Both DAC channels share the same timer, meaning they MUST use the same sampling frequency
 * - The timer runs continuously once initialized
 * - Calling Enable() starts DMA transfers, not the timer
 * - In burst mode, the waveform repeats automatically until the burst count is reached
 * - Internally, single wave mode is using burst mode with burst count of 1
 * - After a burst completes, the DAC must be re-enabled to restart output
 */

#include "dac.h"
#include "timer.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */

// Describes per DAC channel runtime state
typedef struct {
    // Number of completed DMA buffer transfers in the current burst
    volatile uint16_t transfers_done_in_burst;

    // Flag when burst transfer is completed
    volatile bool burst_complete;
} DacState;

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

static volatile DAC_ERROR error = DAC_ERROR_NO_ERRORS;

static SensEdu_DAC_Settings dac_settings[2] = {
    {DAC_CH1, 0, NULL,
        0, SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0},
    {DAC_CH2, 0, NULL,
        0, SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0}
};
static DacState dac_states[2];

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static inline bool is_valid_channel(DAC_Channel* ch);
static SensEdu_DAC_Settings* get_dac_settings(DAC_Channel* dac_channel);
static DacState* get_dac_state(DAC_Channel* dac_channel);
static DAC_ERROR check_settings(SensEdu_DAC_Settings* settings);
static bool is_sr_mismatched(void);
static void dac_init(DAC_Channel* dac_channel);
static uint16_t get_cr_shift(DAC_Channel* dac_channel);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

// Initializes DAC with given settings structure.
void SensEdu_DAC_Init(SensEdu_DAC_Settings* settings) {

    // Sanity checks
    if (settings == NULL) {
        error = DAC_ERROR_NULL_INPUT_SETTINGS;
        return;
    }
    error = check_settings(settings);
    if (error != DAC_ERROR_NO_ERRORS) {
        return;
    }

    // Transform single wave mode into burst with 1 transfer
    if (settings->wave_mode == SENSEDU_DAC_MODE_SINGLE_WAVE) {
        settings->wave_mode = SENSEDU_DAC_MODE_BURST_WAVE;
        settings->burst_num = 1U;
    }

    // Store settings locally
    SensEdu_DAC_Settings* local_settings = get_dac_settings(settings->dac_channel);
    if (local_settings == NULL) return;
    *local_settings = *settings;

    // Check sampling frequency mismatch
    if (is_sr_mismatched()) {
        error = DAC_ERROR_INIT_SAMPLING_FREQ_MISMATCH;
        return;
    }

    // Init flags and storage
    DacState* dac_state = get_dac_state(settings->dac_channel);
    if (dac_state == NULL) return;
    dac_state->transfers_done_in_burst = 0;
    dac_state->burst_complete = false;

    // Initialize timer for DAC sampling (both DAC channels use the same timer)
    TIMER_DAC1Init(settings->sampling_freq);

    // Configure DAC channel
    dac_init(settings->dac_channel);

    // Enable DMA for selected DAC channel
    DMA_InitDmaForDac(settings->dac_channel, settings->mem_address, settings->mem_size, settings->wave_mode);

    // Enable DAC timer (always runs even if dac/dma is off)
    TIMER_DAC1Enable();
}

// Enables DAC peripheral, starting the DMA transfers.
void SensEdu_DAC_Enable(DAC_Channel* dac_channel) {
    if (!is_valid_channel(dac_channel)) {
        error = DAC_ERROR_WRONG_DAC_CHANNEL;
        return;
    }

    DMA_EnableDmaForDac(dac_channel);

    uint16_t shift = get_cr_shift(dac_channel);
    SET_BIT(DAC1->CR, DAC_CR_EN1 << shift);

    uint32_t timeout = UINT32_MAX;
    while (!READ_BIT(DAC1->CR, DAC_CR_EN1 << shift) && timeout--) {
        __NOP();
    }
    if (timeout == 0) {
        error = DAC_ERROR_TIMEOUT;
        return;
    }
}

// Disables DAC peripheral.
void SensEdu_DAC_Disable(DAC_Channel* dac_channel) {
    if (!is_valid_channel(dac_channel)) {
        error = DAC_ERROR_WRONG_DAC_CHANNEL;
        return;
    }

    uint16_t shift = get_cr_shift(dac_channel);
    CLEAR_BIT(DAC1->CR, DAC_CR_EN1 << shift);

    uint32_t timeout = UINT32_MAX;
    while (READ_BIT(DAC1->CR, DAC_CR_EN1 << shift) && timeout--) {
        __NOP();
    }
    if (timeout == 0) {
        error = DAC_ERROR_TIMEOUT;
        return;
    }

    DMA_DisableDmaForDac(dac_channel);
}

// Poll the burst complete flag.
bool SensEdu_DAC_GetBurstCompleteFlag(DAC_Channel* dac_channel) {
    if (!is_valid_channel(dac_channel)) {
        error = DAC_ERROR_WRONG_DAC_CHANNEL;
        return false;
    }

    DacState* state = get_dac_state(dac_channel);
    if (state == NULL) return false;
    return state->burst_complete;
}

// Set burst complete flag to 0.
void SensEdu_DAC_ClearBurstCompleteFlag(DAC_Channel* dac_channel) {
    if (!is_valid_channel(dac_channel)) {
        error = DAC_ERROR_WRONG_DAC_CHANNEL;
        return;
    }

    DacState* state = get_dac_state(dac_channel);
    if (state == NULL) return;
    state->burst_complete = false;
}

// Returns DAC driver's current error state.
DAC_ERROR DAC_GetError(void) {
    return error;
}

// Write to DAC directly in software without DMA.
void DAC_WriteDataManually(DAC_Channel* dac_channel, uint16_t data) {
    if (!is_valid_channel(dac_channel)) {
        error = DAC_ERROR_WRONG_DAC_CHANNEL;
        return;
    }

    if (dac_channel == DAC_CH1) {
        WRITE_REG(DAC1->DHR12R1, data);
    }
    else if (dac_channel == DAC_CH2) {
        WRITE_REG(DAC1->DHR12R2, data);
    }
}

// Read the DAC current data register.
uint16_t DAC_ReadCurrentOutputData(DAC_Channel* dac_channel) {
    if (!is_valid_channel(dac_channel)) {
        error = DAC_ERROR_WRONG_DAC_CHANNEL;
        return 0;
    }

    if (dac_channel == DAC_CH1) {
        return READ_REG(DAC1->DOR1);
    }
    if (dac_channel == DAC_CH2) {
        return READ_REG(DAC1->DOR2);
    }
    error = DAC_ERROR_WRONG_DAC_CHANNEL;
    return 0;
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */

static inline bool is_valid_channel(DAC_Channel* ch) {
    return (ch == DAC_CH1 || ch == DAC_CH2);
}

static SensEdu_DAC_Settings* get_dac_settings(DAC_Channel* dac_channel) {
    if (dac_channel == DAC_CH1) return &dac_settings[0];
    if (dac_channel == DAC_CH2) return &dac_settings[1];
    error = DAC_ERROR_WRONG_DAC_CHANNEL;
    return NULL;
}

static DacState* get_dac_state(DAC_Channel* dac_channel) {
    if (dac_channel == DAC_CH1) return &dac_states[0];
    if (dac_channel == DAC_CH2) return &dac_states[1];
    error = DAC_ERROR_WRONG_DAC_CHANNEL;
    return NULL;
}

static DAC_ERROR check_settings(SensEdu_DAC_Settings* settings) {
    if (settings->dac_channel != DAC_CH1 && settings->dac_channel != DAC_CH2) {
        return DAC_ERROR_WRONG_DAC_CHANNEL;
    }

    if (settings->sampling_freq > 15000000) {
        return DAC_ERROR_INIT_SAMPLING_FREQ_TOO_HIGH;
    }

    if (settings->mem_address == NULL) {
        return DAC_ERROR_INIT_DMA_MEMORY;
    }

    if (settings->mem_size == 0) {
        return DAC_ERROR_INIT_DMA_MEMORY;
    }

    if (settings->wave_mode == SENSEDU_DAC_MODE_BURST_WAVE && settings->burst_num < 1) {
        uint16_t burst = (settings->burst_num < 1) ? 1 : settings->burst_num;
        settings->burst_num = burst;
        return DAC_ERROR_INIT_CYCLE_NUM;
    }

    return DAC_ERROR_NO_ERRORS;
}

// Since both DAC channels use the same timer,
// user is expected to put the same sampling frequency in both of them.
static bool is_sr_mismatched(void) {
    if (dac_settings[0].sampling_freq != 0 && dac_settings[1].sampling_freq != 0) {
        if (dac_settings[0].sampling_freq != dac_settings[1].sampling_freq) {
            return true;
        }
    }
    return false;
}

static void dac_init(DAC_Channel* dac_channel) {

    uint16_t shift = get_cr_shift(dac_channel);

    // Prevent reconfiguration while channel is enabled
    if (READ_BIT(DAC1->CR, ((DAC_CR_EN1 << shift) | (DAC_CR_CEN1 << shift)))) { 
        error = DAC_ERROR_ALREADY_ENABLED;
        return;
    }

    // GPIO
    if (dac_channel == DAC_CH1) {
        MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE4, (0b11) << GPIO_MODER_MODE4_Pos);
    } else if (dac_channel == DAC_CH2) {
        MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE5, (0b11) << GPIO_MODER_MODE5_Pos);
    } else {
        error = DAC_ERROR_WRONG_DAC_CHANNEL;
        return;
    }

    // Clock
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_DAC12EN);

    // DMA
    SET_BIT(DAC1->CR, DAC_CR_DMAUDRIE1 << shift);   // Enable DMA Underrun Interrupt
    SET_BIT(DAC1->CR, DAC_CR_DMAEN1 << shift);      // Enable DMA

    // Trigger
    MODIFY_REG(DAC1->CR, DAC_CR_TSEL1 << shift, (3U) << (DAC_CR_TSEL1_Pos + shift)); // dac_chx_trg3 -> tim4_trgo
    SET_BIT(DAC1->CR, DAC_CR_TEN1 << shift); // Enable Trigger

    // Channel Mode
    MODIFY_REG(DAC1->MCR, DAC_MCR_MODE1 << shift, (0b000) << (DAC_MCR_MODE1_Pos + shift)); // Connected to external pin with buffer enabled
}

// Get CR register shift depending on selected DAC channel.
static uint16_t get_cr_shift(DAC_Channel* dac_channel) {
    uint16_t shift = 0U;
    if (dac_channel == DAC_CH2) {
        // All bits in CR register are shifted by 16 for channel 2
        shift = 16U;
    }
    return shift;
}

// SensEdu_DAC_Enable version that avoids blocking wait to be used inside interrupts
static void dac_enable_hw(DAC_Channel* dac_channel) {
    DMA_EnableDmaForDac(dac_channel);
    uint16_t shift = get_cr_shift(dac_channel);
    SET_BIT(DAC1->CR, DAC_CR_EN1 << shift);
}

/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */

void DAC_IRQHandler(void) {
    if (READ_BIT(DAC1->SR, DAC_SR_DMAUDR1)) {
        SET_BIT(DAC1->SR, DAC_SR_DMAUDR1);
        error = DAC_ERROR_DMA_UNDERRUN;
    }

    if (READ_BIT(DAC1->SR, DAC_SR_DMAUDR2)) {
        SET_BIT(DAC1->SR, DAC_SR_DMAUDR2);
        error = DAC_ERROR_DMA_UNDERRUN;
    }
}

void DAC_TransferCompleteDmaInterrupt(DAC_Channel* dac_channel) {
    SensEdu_DAC_Settings* settings = get_dac_settings(dac_channel);
    if (settings == NULL) return;
    if (settings->wave_mode == SENSEDU_DAC_MODE_BURST_WAVE) {
        DacState* state = get_dac_state(dac_channel);
        if (state == NULL) return;
        (state->transfers_done_in_burst)++;
        if ((state->transfers_done_in_burst) == settings->burst_num) {
            (state->transfers_done_in_burst) = 0;
            (state->burst_complete) = true;
        } else {
            dac_enable_hw(dac_channel);
        }
    }
}
