#include "dma.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static DMA_ERROR error = DMA_ERROR_NO_ERRORS;
static uint8_t transfer_status = 0;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void dma_init(uint16_t* memory0_address);
void dmamux_init(void);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
DMA_ERROR DMA_GetError(void) {
    return error;
}

uint8_t DMA_GetTransferStatus(void) {
    return transfer_status;
}

void DMA_SetTransferStatus(uint8_t new_status) {
    transfer_status = new_status;
}

void DMA_InitPeriph(uint16_t* memory0_address) {
    dma_init(memory0_address);
    dmamux_init();
}

void DMA_EnablePeriph(void) {
    
    //delete this
    SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF4);
    SET_BIT(DMA1->HIFCR, DMA_HIFCR_CHTIF4);
    SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF4);

    if (READ_REG(DMA1->LISR) | READ_REG(DMA1->HISR)) {
        error = DMA_ERROR_INTERRUPTS_NOT_CLEARED;
    }

    SET_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);
}

void DMA_DisablePeriph(void) {
    /*
    1. If the stream is enabled, disable it by resetting the EN bit in the DMA_SxCR register, 
	then read this bit in order to confirm that there is no ongoing stream operation. Writing 
	this bit to 0 is not immediately effective since it is actually written to 0 once all the 
	current transfers are finished. When the EN bit is read as 0, this means that the stream 
	is ready to be configured. It is therefore necessary to wait for the EN bit to be cleared 
	before starting any stream configuration. All the stream dedicated bits set in the status 
	register (DMA_LISR and DMA_HISR) from the previous data block DMA transfer must 
	be cleared before the stream can be re-enabled.
    */
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
void dma_init(uint16_t* memory0_address) {
    
    if (READ_BIT(DMA1_Stream4->CR, DMA_SxCR_EN)) {
        error = DMA_ERROR_ENABLED_BEFORE_INIT;
    }

    // Clock
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);  // DMA1 Clock

    // Priority
    MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_PL, 0b10 << DMA_SxCR_PL_Pos); // High Priority

    // Half-word (16bit) data sizes
    MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_MSIZE, 0b01 << DMA_SxCR_MSIZE_Pos); // memory 0b01
    MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_PSIZE, 0b01 << DMA_SxCR_PSIZE_Pos); // peripheral 0b01

    // Address incrementation
    SET_BIT(DMA1_Stream4->CR, DMA_SxCR_MINC); // memory
    CLEAR_BIT(DMA1_Stream4->CR, DMA_SxCR_PINC); // peripheral

    // Circular mode
    SET_BIT(DMA1_Stream4->CR, DMA_SxCR_CIRC); // ON

    // Data transfer direction
    MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_DIR, 0b00 << DMA_SxCR_DIR_Pos); // peripheral -> memory

    // Enable Interrupts
    SET_BIT(DMA1_Stream4->CR, DMA_SxCR_TCIE); // transfer complete
    //SET_BIT(DMA1_Stream4->CR, DMA_SxCR_TEIE); // transfer error
    //SET_BIT(DMA1_Stream4->CR, DMA_SxCR_DMEIE); // direct mode error
    NVIC_SetPriority(DMA1_Stream4_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Stream4_IRQn);

    // Number of data items to transfer
    MODIFY_REG(DMA1_Stream4->NDTR, DMA_SxNDT, (10U) << DMA_SxNDT_Pos);
    
    // Peripheral data register address
    //MODIFY_REG(DMA1_Stream4->PAR, DMA_SxPAR_PA, (ADC1_BASE + 0x40UL));
    //WRITE_REG(DMA1_Stream4->PAR, (uint32_t)&(ADC1->DR));
    WRITE_REG(DMA1_Stream4->PAR, 0x40022040);
    //MODIFY_REG(DMA1_Stream4->PAR, DMA_SxPAR_PA, &(ADC1->DR) << DMA_SxPAR_PA_Pos); // is it right??
    //MODIFY_REG(DMA1_Stream4->PAR, DMA_SxPAR_PA, (uint32_t)&ADC1->DR); // is it right??

    // Memory data register address
    WRITE_REG(DMA1_Stream4->M0AR, (uint32_t)memory0_address); // is it right??

    CLEAR_BIT(DMA1_Stream4->FCR, DMA_SxFCR_DMDIS); // fifo disable
    
    //HAL_ADC_Start_DMA();
}

void dmamux_init(void) {
    // DMAMUX1 request ID
    MODIFY_REG(DMAMUX1_Channel4->CCR, DMAMUX_CxCR_DMAREQ_ID, (9U) << DMAMUX_CxCR_DMAREQ_ID_Pos); // ADC1 mapping LL_DMAMUX1_REQ_ADC1
}

/*
void DMA1_Stream4_IRQHandler(void) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF4)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF4);
        transfer_status = 1;
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_TEIF4)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF4);
        error = DMA_ERROR_INTERRUPT_TRANSFER_ERROR;
    }
}*/

