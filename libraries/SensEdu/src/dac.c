#include "dac.h"
#include "timer.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static volatile DAC_ERROR error = DAC_ERROR_NO_ERRORS;


/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void dac_init(void);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
DAC_ERROR DAC_GetError(void) {
    return error;
}

void DAC_InitPeriph(void) {
    dac_init();
}

void DAC_EnablePeriph(void) {
    SET_BIT(DAC1->CR, DAC_CR_EN1);
}

void DAC_DisablePeriph(void) {
    CLEAR_BIT(DAC1->CR, DAC_CR_EN1);
}

void DAC_TriggerOutput(void) {
    //SET_BIT(DAC1->SWTRIGR, DAC_SWTRIGR_SWTRIG1); // Software Trigger

    TIMER_DACtrigger_Enable();
}

void DAC_WriteData(uint16_t data) {
    WRITE_REG(DAC1->DHR12R1, data); // Write right aligned data reg
}

uint16_t DAC_ReadOutput(void) {
    return READ_REG(DAC1->DOR1);
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
void dac_init(void) {

    if (READ_BIT(DAC1->CR, DAC_CR_EN1 | DAC_CR_CEN1)) {
        error = DAC_ERROR_ENABLED_BEFORE_INIT;
        return;
    }

    // GPIO
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE4, (0b11) << GPIO_MODER_MODE4_Pos);

    // Clock
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_DAC12EN);

    // DMA
    SET_BIT(DAC1->CR, DAC_CR_DMAUDRIE1); // Enable DMA Underrun Interrupt
    SET_BIT(DAC1->CR, DAC_CR_DMAEN1); // Enable DMA

    // Trigger
    MODIFY_REG(DAC1->CR, DAC_CR_TSEL1, (3U) << DAC_CR_TSEL1_Pos); // dac_chx_trg3 -> tim4_trgo
    SET_BIT(DAC1->CR, DAC_CR_TEN1); // Enable Trigger

    // Channel Mode
    MODIFY_REG(DAC1->MCR, DAC_MCR_MODE1, (0b010) << DAC_MCR_MODE1_Pos); // Connected to external pin with buffer disabled
}

void DAC_IRQHandler(void) {
    if (READ_BIT(DAC1->SR, DAC_SR_DMAUDR1)) {
        SET_BIT(DAC1->SR, DAC_SR_DMAUDR1);
        error = DAC_ERROR_DMA_UNDERRUN;
    }
}
