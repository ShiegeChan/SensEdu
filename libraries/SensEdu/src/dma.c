#include "dma.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */
typedef struct {
    uint32_t clear_flags;
    uint32_t flags;
} DMA_Flags;

typedef struct {
    volatile uint8_t transfer_status;
    uint16_t* memory_address;
    uint16_t memory_size;
    uint32_t adc_reg_address;
    DMA_Stream_TypeDef* dma_stream;
    DMA_Flags* dma_stream_flags;
    IRQn_Type dma_irq;
    DMAMUX_Channel_TypeDef* dmamux_ch;
    uint8_t dmamux_periph_id;
} adc_config;


/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static DMA_Flags dma_ch4_flags = {(DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4), (DMA_HISR_TCIF4 | DMA_HISR_HTIF4 | DMA_HISR_TEIF4 | DMA_HISR_DMEIF4)};
static DMA_Flags dma_ch5_flags = {(DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5), (DMA_HISR_TCIF5 | DMA_HISR_HTIF5 | DMA_HISR_TEIF5 | DMA_HISR_DMEIF5)};
static DMA_Flags dma_ch6_flags = {(DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6), (DMA_HISR_TCIF6 | DMA_HISR_HTIF6 | DMA_HISR_TEIF6 | DMA_HISR_DMEIF6)};
static DMA_Flags dma_ch7_flags = {(DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7), (DMA_HISR_TCIF7 | DMA_HISR_HTIF7 | DMA_HISR_TEIF7 | DMA_HISR_DMEIF7)};

static volatile DMA_ERROR error = DMA_ERROR_NO_ERRORS;

static adc_config adc1_config = {0, (uint16_t*)0x0000, 0, (uint32_t)&(ADC1->DR), DMA1_Stream6, 
    &dma_ch6_flags, DMA1_Stream6_IRQn, DMAMUX1_Channel6, (9U)};
static adc_config adc2_config = {0, (uint16_t*)0x0000, 0, (uint32_t)&(ADC2->DR), DMA1_Stream5,
    &dma_ch5_flags, DMA1_Stream5_IRQn, DMAMUX1_Channel5, (10U)};
static adc_config adc3_config = {0, (uint16_t*)0x0000, 0, (uint32_t)&(ADC3->DR)};


/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void dma_adc_init(adc_config* config);
void dma_dac1_init(DMA_Stream_TypeDef* dma_stream, IRQn_Type dma_irq, 
    uint32_t periph_address, uint32_t mem_address, const uint16_t mem_size, SENSEDU_DAC_MODE wave_mode);

void dma_clear_status_flags(DMA_Flags* dma_flags);
void dma_disable(DMA_Stream_TypeDef* dma_stream, DMA_Flags* flags);
adc_config* get_adc_config(ADC_TypeDef* adc);

void dma_dac_mpu_config(uint16_t* mem_address, const uint16_t mem_size);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
uint8_t SensEdu_DMA_GetADCTransferStatus(ADC_TypeDef* adc) {
    return get_adc_config(adc)->transfer_status;
}

void SensEdu_DMA_ClearADCTransferStatus(ADC_TypeDef* adc) {
    get_adc_config(adc)->transfer_status = 0;
}

DMA_ERROR DMA_GetError(void) {
    return error;
}

void DMA_ADCInit(ADC_TypeDef* adc, uint16_t* mem_address, const uint16_t mem_size) {
    adc_config* config = get_adc_config(adc);
    config->memory_address = mem_address;
    config->memory_size = mem_size;

    dma_adc_init(config);
    MODIFY_REG(config->dmamux_ch->CCR, DMAMUX_CxCR_DMAREQ_ID, config->dmamux_periph_id << DMAMUX_CxCR_DMAREQ_ID_Pos); 
}

void DMA_DAC1Init(uint16_t* mem_address, const uint16_t mem_size, SENSEDU_DAC_MODE wave_mode) {
    dma_dac1_init(DMA1_Stream7, DMA1_Stream7_IRQn, 
        (uint32_t)&(DAC1->DHR12R1), (uint32_t)mem_address, mem_size, wave_mode);
    MODIFY_REG(DMAMUX1_Channel7->CCR, DMAMUX_CxCR_DMAREQ_ID, (67U) << DMAMUX_CxCR_DMAREQ_ID_Pos); 

    // disable cache for dac's dma buffer
    if (mem_size < 1) {
        error = DMA_ERROR_DAC_BUFFER_SIZE_TOO_SMALL;
        return;
    }
    dma_dac_mpu_config(mem_address, mem_size);
}

void DMA_ADCEnable(ADC_TypeDef* adc) {
    adc_config* config = get_adc_config(adc);

    if (config->memory_address == 0x0000 || config->memory_size < 1) {
        error = DMA_ERROR_ADC_WRONG_INPUT;
    }

    // check if the size is the multiple of D-Cache line size (32 words)
    // 16bit - half words -> x2 multiplication
    if ((config->memory_size % (__SCB_DCACHE_LINE_SIZE << 1)) != 0) {
        error = DMA_ERROR_MEMORY_WRONG_SIZE;
        return;
    }

    // cache must be invalidated before reading transferred data
    // second argument in bytes
    SCB_InvalidateDCache_by_Addr(config->memory_address, config->memory_size << 1);

    dma_clear_status_flags(config->dma_stream_flags);
    SET_BIT(config->dma_stream->CR, DMA_SxCR_EN);
}

void DMA_DAC1Enable(void) {
    if (READ_BIT(DMA1_Stream7->CR, DMA_SxCR_EN)) {
        error = DMA_ERROR_ENABLED_BEFORE_ENABLE;
    } 

    dma_clear_status_flags(&dma_ch7_flags);
    SET_BIT(DMA1_Stream7->CR, DMA_SxCR_EN);
}

void DMA_ADCDisable(ADC_TypeDef* adc) {
    adc_config* config = get_adc_config(adc);
    dma_disable(config->dma_stream, config->dma_stream_flags);
}

void DMA_DAC1Disable(void) {
    dma_disable(DMA1_Stream7, &dma_ch7_flags);
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
adc_config* get_adc_config(ADC_TypeDef* adc) {
    if (adc == ADC1) {
        return &adc1_config;
    }
    if (adc == ADC2) {
        return &adc2_config;
    }
    if (adc == ADC3) {
        return &adc3_config;
    }

    error = DMA_ERROR_ADC_WRONG_INPUT;
    return 0;
}

void dma_adc_init(adc_config* config) {
    
    if (READ_BIT(config->dma_stream->CR, DMA_SxCR_EN)) {
        error = DMA_ERROR_ENABLED_BEFORE_INIT;
    }
    
    // Clock
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);  // DMA1 Clock

    // Priority
    MODIFY_REG(config->dma_stream->CR, DMA_SxCR_PL, 0b10 << DMA_SxCR_PL_Pos); // High Priority

    // Half-word (16bit) data sizes
    MODIFY_REG(config->dma_stream->CR, DMA_SxCR_MSIZE, 0b01 << DMA_SxCR_MSIZE_Pos); // memory
    MODIFY_REG(config->dma_stream->CR, DMA_SxCR_PSIZE, 0b01 << DMA_SxCR_PSIZE_Pos); // peripheral

    // Address incrementation
    SET_BIT(config->dma_stream->CR, DMA_SxCR_MINC); // memory
    CLEAR_BIT(config->dma_stream->CR, DMA_SxCR_PINC); // peripheral

    // Circular mode
    SET_BIT(config->dma_stream->CR, DMA_SxCR_CIRC); // ON

    // Data transfer direction
    MODIFY_REG(config->dma_stream->CR, DMA_SxCR_DIR, 0b00 << DMA_SxCR_DIR_Pos); // peripheral -> memory

    // Enable Interrupts
    SET_BIT(config->dma_stream->CR, DMA_SxCR_TCIE); // transfer complete
    SET_BIT(config->dma_stream->CR, DMA_SxCR_TEIE); // transfer error
    NVIC_SetPriority(config->dma_irq, 3);
    NVIC_EnableIRQ(config->dma_irq);

    // Number of data items to transfer
    MODIFY_REG(config->dma_stream->NDTR, DMA_SxNDT, (config->memory_size) << DMA_SxNDT_Pos);
    
    // Peripheral data register address
    WRITE_REG(config->dma_stream->PAR, config->adc_reg_address);

    // Memory data register address
    WRITE_REG(config->dma_stream->M0AR, config->memory_address);
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

    // Disable FIFO
    CLEAR_BIT(dma_stream->FCR, DMA_SxFCR_DMDIS);  
}

void dma_clear_status_flags(DMA_Flags* dma_flags) {
    SET_BIT(DMA1->HIFCR, dma_flags->clear_flags);

    if (READ_BIT(DMA1->HISR, dma_flags->flags)) {
        error = DMA_ERROR_INTERRUPTS_NOT_CLEARED;
    }    
}

void dma_disable(DMA_Stream_TypeDef* dma_stream, DMA_Flags* flags) {
    CLEAR_BIT(dma_stream->CR, DMA_SxCR_EN);
    while(READ_BIT(dma_stream->CR, DMA_SxCR_EN));

    dma_clear_status_flags(flags);
}

void dma_dac_mpu_config(uint16_t* mem_address, const uint16_t mem_size) {
    LL_MPU_Disable();

    // check e.g. LL_MPU_REGION_SIZE_32B mapping 
    // to understand region size calculations
    LL_MPU_ConfigRegion(LL_MPU_REGION_NUMBER5, 0x0, (uint32_t)(mem_address), 
    MPU_REGION_SIZE_ATTRIBUTE(mem_size) | 
    LL_MPU_TEX_LEVEL1 |
    LL_MPU_REGION_FULL_ACCESS |
    LL_MPU_INSTRUCTION_ACCESS_DISABLE | 
    LL_MPU_ACCESS_SHAREABLE |
    LL_MPU_ACCESS_NOT_CACHEABLE |
    LL_MPU_ACCESS_NOT_BUFFERABLE);

    LL_MPU_EnableRegion(LL_MPU_REGION_NUMBER5);
    SCB_CleanDCache_by_Addr((uint32_t)mem_address, mem_size << 1);
    LL_MPU_Enable(LL_MPU_CTRL_PRIVILEGED_DEFAULT);
}


/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */
void DMA1_Stream5_IRQHandler(void) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF5)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF5);
        adc2_config.transfer_status = 1;
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_TEIF5)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF5);
        error = DMA_ERROR_ADC_INTERRUPT_TRANSFER_ERROR;
    }
}

void DMA1_Stream6_IRQHandler(void) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF6)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF6);
        adc1_config.transfer_status = 1;
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
