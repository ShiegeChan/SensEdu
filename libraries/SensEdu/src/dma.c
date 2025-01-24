#include "dma.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */
typedef struct {
    uint32_t clear_flags;
    uint32_t flags
} DMA_Flags;


/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static DMA_Flags dma_ch4_flags = {(DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4), (DMA_HISR_TCIF4 | DMA_HISR_HTIF4 | DMA_HISR_TEIF4 | DMA_HISR_DMEIF4 | DMA_HISR_FEIF4)};
static DMA_Flags dma_ch5_flags = {(DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5), (DMA_HISR_TCIF5 | DMA_HISR_HTIF5 | DMA_HISR_TEIF5 | DMA_HISR_DMEIF5 | DMA_HISR_FEIF5)};
static DMA_Flags dma_ch6_flags = {(DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6), (DMA_HISR_TCIF6 | DMA_HISR_HTIF6 | DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6)};
static DMA_Flags dma_ch7_flags = {(DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7), (DMA_HISR_TCIF7 | DMA_HISR_HTIF7 | DMA_HISR_TEIF7 | DMA_HISR_DMEIF7 | DMA_HISR_FEIF7)};

static volatile DMA_ERROR error = DMA_ERROR_NO_ERRORS;
static volatile uint8_t adc1_transfer_status = 0;       // flag for adc finished transfer
static uint16_t* adc1_memory_address = 0x0000;
static uint16_t adc1_memory_size = 0;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void dma_adc1_init(DMA_Stream_TypeDef* dma_stream, IRQn_Type dma_irq, 
    uint32_t periph_address, uint32_t mem_address, const uint16_t mem_size);
void dma_dac1_init(DMA_Stream_TypeDef* dma_stream, IRQn_Type dma_irq, 
    uint32_t periph_address, uint32_t mem_address, const uint16_t mem_size, SENSEDU_DAC_MODE wave_mode);

void dma_clear_status_flags(DMA_Flags dma_flags);
void dma_disable(DMA_Stream_TypeDef* dma_stream, DMA_Flags flags);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
uint8_t SensEdu_DMA_GetADC1TransferStatus(void) {
    return adc1_transfer_status;
}

void SensEdu_DMA_ClearADC1TransferStatus(void) {
    adc1_transfer_status = 0;
}

DMA_ERROR DMA_GetError(void) {
    return error;
}

void DMA_ADC1Init(uint16_t* mem_address, const uint16_t mem_size) {
    adc1_memory_address = mem_address;
    adc1_memory_size = mem_size;

    dma_adc1_init(DMA1_Stream6, DMA1_Stream6_IRQn, (uint32_t)&(ADC1->DR), (uint32_t)mem_address, mem_size);
    MODIFY_REG(DMAMUX1_Channel6->CCR, DMAMUX_CxCR_DMAREQ_ID, (9U) << DMAMUX_CxCR_DMAREQ_ID_Pos); 
}

void DMA_DAC1Init(uint16_t* mem_address, const uint16_t mem_size, SENSEDU_DAC_MODE wave_mode) {
    dma_dac1_init(DMA1_Stream7, DMA1_Stream7_IRQn, 
        (uint32_t)&(DAC1->DHR12R1), (uint32_t)mem_address, mem_size, wave_mode);
    MODIFY_REG(DMAMUX1_Channel7->CCR, DMAMUX_CxCR_DMAREQ_ID, (67U) << DMAMUX_CxCR_DMAREQ_ID_Pos); 
}

void DMA_ADC1Enable(void) {
    if (adc1_memory_address == 0x0000 || adc1_memory_size < 1) {
        error = DMA_ERROR_ADC1_ENABLE_BEFORE_INIT;
    }

    // check if the size is the multiple of D-Cache line size (32 words)
    // 16bit - half words -> x2 multiplication
    if ((adc1_memory_size % (__SCB_DCACHE_LINE_SIZE << 1)) != 0) {
        error = DMA_ERROR_MEMORY_WRONG_SIZE;
        return;
    }

    // cache must be invalidated before reading transferred data
    // second argument in bytes
    SCB_InvalidateDCache_by_Addr(adc1_memory_address, adc1_memory_size << 1);

    dma_clear_status_flags(dma_ch6_flags);
    SET_BIT(DMA1_Stream6->CR, DMA_SxCR_EN);
}

void DMA_DAC1Enable(void) {
    if (!READ_BIT(DMA1_Stream7->CR, DMA_SxCR_EN)) {
        dma_clear_status_flags(dma_ch7_flags);
    } 

    SET_BIT(DMA1_Stream7->CR, DMA_SxCR_EN);
}

void DMA_ADC1Disable(void) {
    dma_disable(DMA1_Stream6, dma_ch6_flags);
}

void DMA_DAC1Disable(void) {
    dma_disable(DMA1_Stream7, dma_ch7_flags);
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
void dma_adc1_init(DMA_Stream_TypeDef* dma_stream, IRQn_Type dma_irq,
    uint32_t periph_address, uint32_t mem_address, const uint16_t mem_size) {
    
    if (READ_BIT(dma_stream->CR, DMA_SxCR_EN)) {
        error = DMA_ERROR_ENABLED_BEFORE_INIT;
    }
    
    // Clock
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);  // DMA1 Clock

    // Priority
    MODIFY_REG(dma_stream->CR, DMA_SxCR_PL, 0b10 << DMA_SxCR_PL_Pos); // High Priority

    // Half-word (16bit) data sizes
    MODIFY_REG(dma_stream->CR, DMA_SxCR_MSIZE, 0b01 << DMA_SxCR_MSIZE_Pos); // memory
    MODIFY_REG(dma_stream->CR, DMA_SxCR_PSIZE, 0b01 << DMA_SxCR_PSIZE_Pos); // peripheral

    // Address incrementation
    SET_BIT(dma_stream->CR, DMA_SxCR_MINC); // memory
    CLEAR_BIT(dma_stream->CR, DMA_SxCR_PINC); // peripheral

    // Circular mode
    SET_BIT(dma_stream->CR, DMA_SxCR_CIRC); // ON

    // Data transfer direction
    MODIFY_REG(dma_stream->CR, DMA_SxCR_DIR, 0b00 << DMA_SxCR_DIR_Pos); // peripheral -> memory

    // Enable Interrupts
    SET_BIT(dma_stream->CR, DMA_SxCR_TCIE); // transfer complete
    SET_BIT(dma_stream->CR, DMA_SxCR_TEIE); // transfer error
    NVIC_SetPriority(dma_irq, 3);
    NVIC_EnableIRQ(dma_irq);

    // Number of data items to transfer
    MODIFY_REG(dma_stream->NDTR, DMA_SxNDT, (mem_size) << DMA_SxNDT_Pos);
    
    // Peripheral data register address
    WRITE_REG(dma_stream->PAR, periph_address);

    // Memory data register address
    WRITE_REG(dma_stream->M0AR, mem_address);
}

void dma_dac1_init(DMA_Stream_TypeDef* dma_stream, IRQn_Type dma_irq, 
    uint32_t periph_address, uint32_t mem_address, const uint16_t mem_size, SENSEDU_DAC_MODE wave_mode) {
    
    if (READ_BIT(dma_stream->CR, DMA_SxCR_EN)) {
        error = DMA_ERROR_ENABLED_BEFORE_INIT;
    }
    
    // Clock
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);  // DMA1 Clock

    // Priority
    MODIFY_REG(dma_stream->CR, DMA_SxCR_PL, 0b11 << DMA_SxCR_PL_Pos); // Very High Priority

    // Half-word (16bit) data sizes
    MODIFY_REG(dma_stream->CR, DMA_SxCR_MSIZE, 0b01 << DMA_SxCR_MSIZE_Pos); // memory
    MODIFY_REG(dma_stream->CR, DMA_SxCR_PSIZE, 0b01 << DMA_SxCR_PSIZE_Pos); // peripheral

    // Address incrementation
    SET_BIT(dma_stream->CR, DMA_SxCR_MINC); // memory
    CLEAR_BIT(dma_stream->CR, DMA_SxCR_PINC); // peripheral

    if (wave_mode == SENSEDU_DAC_MODE_CONTINUOUS_WAVE) {
        SET_BIT(dma_stream->CR, DMA_SxCR_CIRC); // Circular mode
    } else {
        CLEAR_BIT(dma_stream->CR, DMA_SxCR_CIRC);
    }

    // Data transfer direction
    MODIFY_REG(dma_stream->CR, DMA_SxCR_DIR, 0b01 << DMA_SxCR_DIR_Pos); // memory -> peripheral

    // Enable Interrupts
    SET_BIT(dma_stream->CR, DMA_SxCR_TCIE); // transfer complete
    SET_BIT(dma_stream->CR, DMA_SxCR_TEIE); // transfer error
    NVIC_SetPriority(dma_irq, 3);
    NVIC_EnableIRQ(dma_irq);

    // Number of data items to transfer
    MODIFY_REG(dma_stream->NDTR, DMA_SxNDT, (mem_size) << DMA_SxNDT_Pos);
    
    // Peripheral data register address
    WRITE_REG(dma_stream->PAR, periph_address);

    // Memory data register address
    WRITE_REG(dma_stream->M0AR, mem_address);
}

void dma_clear_status_flags(DMA_Flags dma_flags) {
    SET_BIT(DMA1->HIFCR, dma_flags.clear_flags);

    // wait till flags are clean
    for (uint16_t i = 0; i < 10000; i++) {
        if (!READ_BIT(DMA1->HISR, dma_flags.flags)) {
            return;
        }
    }

    //error = DMA_ERROR_INTERRUPTS_NOT_CLEARED;
}

void dma_disable(DMA_Stream_TypeDef* dma_stream, DMA_Flags flags) {
    CLEAR_BIT(dma_stream->CR, DMA_SxCR_EN);
    while(READ_BIT(dma_stream->CR, DMA_SxCR_EN));
    dma_clear_status_flags(flags);
}


/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */
void DMA1_Stream6_IRQHandler(void) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF6)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF6);
        adc1_transfer_status = 1;
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_TEIF6)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF6);
        error = DMA_ERROR_ADC_INTERRUPT_TRANSFER_ERROR;
    }
}

void DMA1_Stream7_IRQHandler(void) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF7)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF7);
        DAC_TransferCompleteDMAinterrupt(DAC1);
        
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_TEIF7)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF7);
        error = DMA_ERROR_DAC_INTERRUPT_TRANSFER_ERROR;
    }
}
