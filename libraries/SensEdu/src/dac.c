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

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void dac_init(DAC_TypeDef* dac);
SensEdu_DAC_Settings* get_settings(DAC_TypeDef* dac);
DAC_ERROR check_settings(SensEdu_DAC_Settings* settings);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
// TODO: add timer here too
void SensEdu_DAC_Init(SensEdu_DAC_Settings* dac_settings) {
    if (dac_settings->dac == DAC1) {
        dac1_settings = *dac_settings;
    } else if (dac_settings->dac == DAC2) {
        dac2_settings = *dac_settings;
    } else {
        error = DAC_ERROR_INIT_FAILED;
        return;
    }

    if (check_settings(dac_settings) != DAC_ERROR_NO_ERRORS) {
        error = DAC_ERROR_INIT_FAILED;
        return;
    }

    dac_init(dac_settings->dac);
    if (dac_settings->dac == DAC1) {
        DMA_DAC1Init(dac_settings->mem_address, dac_settings->mem_size);
    } else if (dac_settings->dac == DAC2) {
        //DMA_DAC2Init(mem_address, mem_size);
    }
}

void SensEdu_DAC_Enable(DAC_TypeDef* dac) {
    SET_BIT(dac->CR, DAC_CR_EN1);
    if (dac == DAC1) {
        DMA_DAC1Enable();
    } else if (dac == DAC2) {
        //DMA_DAC2Enable();
    }
}

void SensEdu_DAC_Disable(DAC_TypeDef* dac) {
    if (dac == DAC1) {
        DMA_DAC1Disable();
        //DAC1->DHR12R1 = 0U; TODO: investigate the end of dac waves, do they need extra zero?
    } else if (dac == DAC2) {
        //DMA_DAC2Disable();
        //DAC2->DHR12R1 = 0U;
    }

    CLEAR_BIT(dac->CR, DAC_CR_EN1);
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

DAC_ERROR check_settings(SensEdu_DAC_Settings* settings) {
    if (settings->dac != DAC1 && settings->dac != DAC2) {
        return DAC_ERROR_INIT_FAILED;
    } else if (settings->mem_address == 0x0000) {
        return DAC_ERROR_INIT_FAILED;
    } else if (settings->mem_address == 0) {
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
