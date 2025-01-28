#include "dac.h"
#include "timer.h"

#define DAC2 DAC1 // TODO: redo with proper channel

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static volatile DAC_ERROR error = DAC_ERROR_NO_ERRORS;

static SensEdu_DAC_Settings dac1_settings = {DAC1, 1000000, 0x0000, 0, 
    SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0};
static SensEdu_DAC_Settings dac2_settings = {DAC2, 1000000, 0x0000, 0, 
    SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0};

static volatile uint16_t dac_transfer_cnt = 0;  // current written wave cycle to dac
static volatile uint8_t dac_burst_complete = 0;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void dac_init(DAC_TypeDef* dac);
SensEdu_DAC_Settings* get_settings(DAC_TypeDef* dac);
static DAC_ERROR check_settings(SensEdu_DAC_Settings* settings);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
void SensEdu_DAC_Init(SensEdu_DAC_Settings* dac_settings) {
    // Storing settings
    if (dac_settings->dac == DAC1) {
        dac1_settings = *dac_settings;
    } else if (dac_settings->dac == DAC2) {
        dac2_settings = *dac_settings;
    } else {
        error = DAC_ERROR_INIT_FAILED;
        return;
    }

    // Sanity checks
    if (check_settings(dac_settings) != DAC_ERROR_NO_ERRORS) {
        error = DAC_ERROR_INIT_FAILED;
        return;
    }

    // Timer + DAC + DMA
    TIMER_DAC1Init(dac_settings->sampling_freq);
    dac_init(dac_settings->dac);
    if (dac_settings->dac == DAC1) {
        DMA_DAC1Init(dac_settings->mem_address, dac_settings->mem_size, dac_settings->wave_mode);
    } else if (dac_settings->dac == DAC2) {
        //DMA_DAC2Init(mem_address, mem_size);
    }

    // Enable Timer (it always runs even if dac/dma is off)
    TIMER_DAC1Enable();
}

void SensEdu_DAC_Enable(DAC_TypeDef* dac) {
    DMA_DAC1Enable();

    SET_BIT(dac->CR, DAC_CR_EN1);
    while(!READ_BIT(dac->CR, DAC_CR_EN1));
}

void SensEdu_DAC_Disable(DAC_TypeDef* dac) {
    CLEAR_BIT(dac->CR, DAC_CR_EN1);
    while(READ_BIT(dac->CR, DAC_CR_EN1));

    DMA_DAC1Disable();
}

uint8_t SensEdu_DAC_GetBurstCompleteFlag(void) {
    return dac_burst_complete;
}

void SensEdu_DAC_ClearBurstCompleteFlag(void) {
    dac_burst_complete = 0;
}

DAC_ERROR DAC_GetError(void) {
    return error;
}

void DAC_WriteDataManually(DAC_TypeDef* dac, uint16_t data) {
    WRITE_REG(dac->DHR12R1, data);
}

uint16_t DAC_ReadCurrentOutputData(DAC_TypeDef* dac) {
    return READ_REG(dac->DOR1);
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
// TODO: fix it to work with DAC2 too
void dac_init(DAC_TypeDef* dac) {

    if (READ_BIT(dac->CR, DAC_CR_EN1 | DAC_CR_CEN1)) {
        error = DAC_ERROR_ENABLED_BEFORE_INIT;
        return;
    }

    // TODO: check if it is needed and change according to DAC channel
    // GPIO 
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE4, (0b11) << GPIO_MODER_MODE4_Pos);

    // Clock
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_DAC12EN);

    // DMA
    SET_BIT(dac->CR, DAC_CR_DMAUDRIE1); // Enable DMA Underrun Interrupt
    SET_BIT(dac->CR, DAC_CR_DMAEN1); // Enable DMA

    // Trigger
    MODIFY_REG(dac->CR, DAC_CR_TSEL1, (3U) << DAC_CR_TSEL1_Pos); // dac_chx_trg3 -> tim4_trgo
    SET_BIT(dac->CR, DAC_CR_TEN1); // Enable Trigger

    // Channel Mode
    MODIFY_REG(dac->MCR, DAC_MCR_MODE1, (0b010) << DAC_MCR_MODE1_Pos); // Connected to external pin with buffer disabled
}

SensEdu_DAC_Settings* get_settings(DAC_TypeDef* dac) {
    if (dac == DAC1) {
        return &dac1_settings;
    } else if (dac == DAC2) {
        return &dac2_settings;
    } else {
        error = DAC_ERROR_WRONG_CHANNEL;
    }
}

static DAC_ERROR check_settings(SensEdu_DAC_Settings* settings) {
    if (settings->dac != DAC1 && settings->dac != DAC2) {
        return DAC_ERROR_INIT_FAILED;
    } else if (settings->mem_address == 0x0000) {
        return DAC_ERROR_INIT_FAILED;
    } else if (settings->mem_address == 0) {
        return DAC_ERROR_INIT_FAILED;
    } else if (settings->wave_mode == SENSEDU_DAC_MODE_BURST_WAVE && settings->burst_num < 1) {
        settings->burst_num = 1; // be careful not to stuck in interrupt
        return DAC_ERROR_INIT_FAILED;
    }

    return DAC_ERROR_NO_ERRORS;
}

/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */
void DAC_IRQHandler(void) {
    if (READ_BIT(DAC1->SR, DAC_SR_DMAUDR1)) {
        SET_BIT(DAC1->SR, DAC_SR_DMAUDR1);
        error = DAC_ERROR_DMA_UNDERRUN;
    }

    if (READ_BIT(DAC2->SR, DAC_SR_DMAUDR1)) {
        SET_BIT(DAC2->SR, DAC_SR_DMAUDR1);
        error = DAC_ERROR_DMA_UNDERRUN;
    }
}

// TODO: make it flexible for all DACs
// maybe rewrite to be only for ch1 for speed
void DAC_TransferCompleteDMAinterrupt(DAC_TypeDef* dac) {
    if (get_settings(dac)->wave_mode == SENSEDU_DAC_MODE_BURST_WAVE) {
        dac_transfer_cnt++;
        if (dac_transfer_cnt == get_settings(dac)->burst_num) {
            dac_transfer_cnt = 0;
            dac_burst_complete = 1;
        } else {
            SensEdu_DAC_Enable(DAC1);
        }
    }
}
