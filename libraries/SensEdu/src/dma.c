/**
 * @file dma.c
 * @brief Internal implementation of DMA driver for ADC and DAC with MPU cache management
 *
 * This module provides:
 * - DMA initialization, configuration, and management for ADC and DAC peripherals
 * - Support for circular and normal mode transfers
 * - MPU cache disabling for DMA buffers
 *
 * Notes:
 * - Hard-coded DMA streams and DMAMUX channels for specific ADC/DAC instances
 * - Buffers must be allocated with proper alignment using the provided macros
 * - Calling DMA_EnableDmaForX multiple times without a preceding disable is unsafe
 */

#include "dma.h"
#include "adc.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */

typedef struct {
    uint32_t ifcr_clear_mask;
    uint32_t isr_status_mask;
} DmaFlagMasks;

typedef struct {
    uint16_t* buf;                      // DMA buffer pointer
    size_t buf_samples;                 // DMA buffer 16-bit elements
    uintptr_t periph_dr_addr;           // Address of the Data Register (DR) of the peripheral DMA is attached to
    DMA_Stream_TypeDef* stream;         // DMA Stream
    const uint8_t stream_idx;           // DMA Stream index
    IRQn_Type irq;                      // IRQ for selected DMA Stream
    DMAMUX_Channel_TypeDef* dmamux_ch;  // DMAMUX channel
    uint8_t dmamux_request_id;          // Peripheral request unique ID
} DmaConfig;

/* -------------------------------------------------------------------------- */
/*                                    Maps                                    */
/* -------------------------------------------------------------------------- */

static const DmaFlagMasks dma_flag_map[] = {
    {(DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0), (DMA_LISR_TCIF0 | DMA_LISR_HTIF0 | DMA_LISR_TEIF0 | DMA_LISR_DMEIF0)},
    {(DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1), (DMA_LISR_TCIF1 | DMA_LISR_HTIF1 | DMA_LISR_TEIF1 | DMA_LISR_DMEIF1)},
    {(DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTEIF2), (DMA_LISR_TCIF2 | DMA_LISR_HTIF2 | DMA_LISR_TEIF2 | DMA_LISR_DMEIF2)},
    {(DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3), (DMA_LISR_TCIF3 | DMA_LISR_HTIF3 | DMA_LISR_TEIF3 | DMA_LISR_DMEIF3)},
    {(DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4), (DMA_HISR_TCIF4 | DMA_HISR_HTIF4 | DMA_HISR_TEIF4 | DMA_HISR_DMEIF4)},
    {(DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5), (DMA_HISR_TCIF5 | DMA_HISR_HTIF5 | DMA_HISR_TEIF5 | DMA_HISR_DMEIF5)},
    {(DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6), (DMA_HISR_TCIF6 | DMA_HISR_HTIF6 | DMA_HISR_TEIF6 | DMA_HISR_DMEIF6)},
    {(DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7), (DMA_HISR_TCIF7 | DMA_HISR_HTIF7 | DMA_HISR_TEIF7 | DMA_HISR_DMEIF7)}
};

static DmaConfig dma_config_map[] = {
    {NULL, 0, (uintptr_t)&(DAC1->DHR12R1), DMA1_Stream2, 2U,
    DMA1_Stream2_IRQn, DMAMUX1_Channel2, (67U)},
    {NULL, 0, (uintptr_t)&(DAC1->DHR12R2), DMA1_Stream3, 3U,
    DMA1_Stream3_IRQn, DMAMUX1_Channel3, (68U)},
    {NULL, 0, (uintptr_t)&(ADC1->DR), DMA1_Stream6, 6U,
    DMA1_Stream6_IRQn, DMAMUX1_Channel6, (9U)},
    {NULL, 0, (uintptr_t)&(ADC2->DR), DMA1_Stream5, 5U,
    DMA1_Stream5_IRQn, DMAMUX1_Channel5, (10U)},
    {NULL, 0, (uintptr_t)&(ADC3->DR), DMA1_Stream7, 7U,
    DMA1_Stream7_IRQn, DMAMUX1_Channel7, (115U)}
};

enum DmaConfigMapIdx {
    DAC_CH1_IDX = 0,
    DAC_CH2_IDX = 1,
    ADC1_IDX = 2,
    ADC2_IDX = 3,
    ADC3_IDX = 4
};

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

// Global error container
static volatile DMA_ERROR error = DMA_ERROR_NO_ERRORS;

// Global MPU region counter, allows up to 4 buffers (4-7)
static uint32_t mpu_region = LL_MPU_REGION_NUMBER4;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static DMA_ERROR check_config(DmaConfig* config);
static DmaConfig* get_adc_config(ADC_TypeDef* adc);
static DmaConfig* get_dac_config(DAC_Channel* dac_ch);

static void init_dma_for_adc(DmaConfig* config);
static void init_dma_for_dac(DmaConfig* config, SENSEDU_DAC_MODE wave_mode);

static void enable_dma(DmaConfig* config);
static void disable_dma(DmaConfig* config);

static void clear_dma_status_flags(const uint8_t stream_idx);

static void disable_cache(uint16_t* buf, size_t buf_samples);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

// Returns DMA error code
DMA_ERROR DMA_GetError(void) {
    return error;
}

void DMA_InitDmaForAdc(ADC_TypeDef* adc, uint16_t* buf, size_t buf_samples) {
    DmaConfig* config = get_adc_config(adc);
    if (!config) return;
    config->buf = buf;
    config->buf_samples = buf_samples;
    DMA_ERROR err = check_config(config);
    if (err != DMA_ERROR_NO_ERRORS) {
        error = err;
        return;
    }

    init_dma_for_adc(config);
    MODIFY_REG(config->dmamux_ch->CCR, DMAMUX_CxCR_DMAREQ_ID, config->dmamux_request_id << DMAMUX_CxCR_DMAREQ_ID_Pos);

    disable_cache(buf, buf_samples);
}

void DMA_InitDmaForDac(DAC_Channel* dac_ch, uint16_t* buf, size_t buf_samples, SENSEDU_DAC_MODE wave_mode) {
    DmaConfig* config = get_dac_config(dac_ch);
    if (!config) return;
    config->buf = buf;
    config->buf_samples = buf_samples;
    DMA_ERROR err = check_config(config);
    if (err != DMA_ERROR_NO_ERRORS) {
        error = err;
        return;
    }

    init_dma_for_dac(config, wave_mode);
    MODIFY_REG(config->dmamux_ch->CCR, DMAMUX_CxCR_DMAREQ_ID, config->dmamux_request_id << DMAMUX_CxCR_DMAREQ_ID_Pos);

    disable_cache(buf, buf_samples);
}

void DMA_EnableDmaForAdc(ADC_TypeDef* adc) {
    enable_dma(get_adc_config(adc));
}

void DMA_EnableDmaForDac(DAC_Channel* dac_ch) {
    enable_dma(get_dac_config(dac_ch));
}

void DMA_DisableDmaForAdc(ADC_TypeDef* adc) {
    disable_dma(get_adc_config(adc));
}

void DMA_DisableDmaForDac(DAC_Channel* dac_ch) {
    disable_dma(get_dac_config(dac_ch));
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */

static DMA_ERROR check_config(DmaConfig* config) {
    if (!config || !config->buf) {
        return DMA_ERROR_INIT;
    }

    if (config->buf_samples == 0 || config->buf_samples > UINT16_MAX) {
        return DMA_ERROR_INIT;
    }

    return DMA_ERROR_NO_ERRORS;
}

static DmaConfig* get_adc_config(ADC_TypeDef* adc) {
    if (adc == ADC1) return &dma_config_map[ADC1_IDX];
    if (adc == ADC2) return &dma_config_map[ADC2_IDX];
    if (adc == ADC3) return &dma_config_map[ADC3_IDX];
    error = DMA_ERROR_ADC_WRONG_INSTANCE;
    return NULL;
}

static DmaConfig* get_dac_config(DAC_Channel* dac_ch) {
    if (dac_ch == DAC_CH1) return &dma_config_map[DAC_CH1_IDX];
    if (dac_ch == DAC_CH2) return &dma_config_map[DAC_CH2_IDX];
    error = DMA_ERROR_DAC_WRONG_INSTANCE;
    return NULL;
}

void init_dma_for_adc(DmaConfig* config) {
    
    if (READ_BIT(config->stream->CR, DMA_SxCR_EN)) {
        error = DMA_ERROR_ENABLED_BEFORE_INIT;
        return;
    }
    
    // Clock
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);  // DMA1 Clock

    // Priority
    MODIFY_REG(config->stream->CR, DMA_SxCR_PL, 0b10 << DMA_SxCR_PL_Pos); // High Priority

    // Disable FIFO
    CLEAR_BIT(config->stream->FCR, DMA_SxFCR_DMDIS);

    // Half-word (16bit) data sizes
    MODIFY_REG(config->stream->CR, DMA_SxCR_MSIZE, 0b01 << DMA_SxCR_MSIZE_Pos); // memory
    MODIFY_REG(config->stream->CR, DMA_SxCR_PSIZE, 0b01 << DMA_SxCR_PSIZE_Pos); // peripheral

    // Address incrementation
    SET_BIT(config->stream->CR, DMA_SxCR_MINC); // memory
    CLEAR_BIT(config->stream->CR, DMA_SxCR_PINC); // peripheral

    // Circular mode
    SET_BIT(config->stream->CR, DMA_SxCR_CIRC); // ON

    // Data transfer direction
    MODIFY_REG(config->stream->CR, DMA_SxCR_DIR, 0b00 << DMA_SxCR_DIR_Pos); // peripheral -> memory

    // Number of data items to transfer
    MODIFY_REG(config->stream->NDTR, DMA_SxNDT, (config->buf_samples) << DMA_SxNDT_Pos);
    
    // Peripheral data register address
    WRITE_REG(config->stream->PAR, config->periph_dr_addr);

    // Memory data register address
    WRITE_REG(config->stream->M0AR, (uintptr_t)config->buf);

    // Enable Interrupts
    SET_BIT(config->stream->CR, DMA_SxCR_TCIE); // transfer complete
    SET_BIT(config->stream->CR, DMA_SxCR_HTIE); // half-transfer reached
    SET_BIT(config->stream->CR, DMA_SxCR_TEIE); // transfer error
    NVIC_SetPriority(config->irq, 3);
    NVIC_EnableIRQ(config->irq);
}

void init_dma_for_dac(DmaConfig* config, SENSEDU_DAC_MODE wave_mode) {
    
    if (READ_BIT(config->stream->CR, DMA_SxCR_EN)) {
        error = DMA_ERROR_ENABLED_BEFORE_INIT;
        return;
    }
    
    // Clock
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN); // DMA1 Clock

    // Priority
    MODIFY_REG(config->stream->CR, DMA_SxCR_PL, 0b11 << DMA_SxCR_PL_Pos); // Very High Priority

    // Half-word (16bit) data sizes
    MODIFY_REG(config->stream->CR, DMA_SxCR_MSIZE, 0b01 << DMA_SxCR_MSIZE_Pos); // memory
    MODIFY_REG(config->stream->CR, DMA_SxCR_PSIZE, 0b01 << DMA_SxCR_PSIZE_Pos); // peripheral

    // Address incrementation
    SET_BIT(config->stream->CR, DMA_SxCR_MINC); // memory
    CLEAR_BIT(config->stream->CR, DMA_SxCR_PINC); // peripheral

    if (wave_mode == SENSEDU_DAC_MODE_CONTINUOUS_WAVE) {
        SET_BIT(config->stream->CR, DMA_SxCR_CIRC); // Circular mode
    } else {
        CLEAR_BIT(config->stream->CR, DMA_SxCR_CIRC);
    }

    // Data transfer direction
    MODIFY_REG(config->stream->CR, DMA_SxCR_DIR, 0b01 << DMA_SxCR_DIR_Pos); // memory -> peripheral

    // Enable Interrupts
    SET_BIT(config->stream->CR, DMA_SxCR_TCIE); // transfer complete
    SET_BIT(config->stream->CR, DMA_SxCR_TEIE); // transfer error
    NVIC_SetPriority(config->irq, 3);
    NVIC_EnableIRQ(config->irq);

    // Number of data items to transfer
    MODIFY_REG(config->stream->NDTR, DMA_SxNDT, (config->buf_samples) << DMA_SxNDT_Pos);

    // Peripheral data register address
    WRITE_REG(config->stream->PAR, config->periph_dr_addr);

    // Memory data register address
    WRITE_REG(config->stream->M0AR, (uintptr_t)config->buf);

    // Disable FIFO
    CLEAR_BIT(config->stream->FCR, DMA_SxFCR_DMDIS);
}

static void enable_dma(DmaConfig* config) {
    if (!config) return;

    clear_dma_status_flags(config->stream_idx);
    SET_BIT(config->stream->CR, DMA_SxCR_EN);
}

static void disable_dma(DmaConfig* config) {
    if (!config) return;

    CLEAR_BIT(config->stream->CR, DMA_SxCR_EN);
    uint32_t timeout = UINT32_MAX;
    while (READ_BIT(config->stream->CR, DMA_SxCR_EN) && timeout--) {
        __NOP();
    }
    if (timeout == 0) {
        error = DMA_ERROR_TIMEOUT;
        return;
    }

    clear_dma_status_flags(config->stream_idx);
}

static void clear_dma_status_flags(const uint8_t stream_idx) {

    if (stream_idx > 7) {
        error = DMA_ERROR_UNEXPECTED_FLAG_MASK;
        return;
    }

    if (stream_idx > 3) {
        SET_BIT(DMA1->HIFCR, dma_flag_map[stream_idx].ifcr_clear_mask);
        if (READ_BIT(DMA1->HISR, dma_flag_map[stream_idx].isr_status_mask)) {
            error = DMA_ERROR_INTERRUPTS_NOT_CLEARED;
        }
        return;
    }

    SET_BIT(DMA1->LIFCR, dma_flag_map[stream_idx].ifcr_clear_mask);
    if (READ_BIT(DMA1->LISR, dma_flag_map[stream_idx].isr_status_mask)) {
        error = DMA_ERROR_INTERRUPTS_NOT_CLEARED;
    }
}

// Config Memory Protection Unit (MPU) to disable cache for selected memory
static void disable_cache(uint16_t* buf, size_t buf_samples) {
    if (mpu_region > LL_MPU_REGION_NUMBER7) {
        error = DMA_ERROR_MPU_NO_REGION_AVAILABLE;
        return;
    }

    size_t region_size_bytes = MPU_NEXT_POWER_OF_2(buf_samples * sizeof(uint16_t));
    uint32_t region_size_attr = MPU_REGION_SIZE_ATTRIBUTE(buf_samples * sizeof(uint16_t));

    // lower 0xFFF must always be 0s
    if (((uintptr_t)buf & (region_size_bytes - 1)) != 0) {
        error = DMA_ERROR_MPU_BASE_NOT_ALIGNED;
        return;
    }

    LL_MPU_Disable();

    // Check MPU mapping (e.g., LL_MPU_REGION_SIZE_32B) to understand the region size calculations
    LL_MPU_ConfigRegion(mpu_region, 0x0, (uintptr_t)(buf),
    region_size_attr |
    LL_MPU_TEX_LEVEL1 |
    LL_MPU_REGION_FULL_ACCESS |
    LL_MPU_INSTRUCTION_ACCESS_DISABLE | 
    LL_MPU_ACCESS_SHAREABLE |
    LL_MPU_ACCESS_NOT_CACHEABLE |
    LL_MPU_ACCESS_NOT_BUFFERABLE);

    LL_MPU_EnableRegion(mpu_region);
    SCB_CleanDCache_by_Addr(buf, region_size_bytes);
    LL_MPU_Enable(LL_MPU_CTRL_PRIVILEGED_DEFAULT);
    mpu_region++;
}

/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */

void DMA1_Stream5_IRQHandler(void) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF5)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF5);
        ADC_SetDmaTransferComplete(ADC2);
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_HTIF5)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CHTIF5);
        ADC_SetDmaHalfTransferComplete(ADC2);
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_TEIF5)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF5);
        error = DMA_ERROR_ADC_INTERRUPT_TRANSFER_ERROR;
    }
}

void DMA1_Stream6_IRQHandler(void) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF6)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF6);
        ADC_SetDmaTransferComplete(ADC1);
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_HTIF6)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CHTIF6);
        ADC_SetDmaHalfTransferComplete(ADC1);
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_TEIF6)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF6);
        error = DMA_ERROR_ADC_INTERRUPT_TRANSFER_ERROR;
    }
}

void DMA1_Stream7_IRQHandler(void) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF7)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF7);
        ADC_SetDmaTransferComplete(ADC3);
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_HTIF7)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CHTIF7);
        ADC_SetDmaHalfTransferComplete(ADC3);
    }

    if (READ_BIT(DMA1->HISR, DMA_HISR_TEIF7)) {
        SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTEIF7);
        error = DMA_ERROR_ADC_INTERRUPT_TRANSFER_ERROR;
    }
}

void DMA1_Stream2_IRQHandler(void) {
    if (READ_BIT(DMA1->LISR, DMA_LISR_TCIF2)) {
        SET_BIT(DMA1->LIFCR, DMA_LIFCR_CTCIF2);
        DAC_TransferCompleteDmaInterrupt(DAC_CH1);
    }

    if (READ_BIT(DMA1->LISR, DMA_LISR_TEIF2)) {
        SET_BIT(DMA1->LIFCR, DMA_LIFCR_CTEIF2);
        error = DMA_ERROR_DAC_INTERRUPT_TRANSFER_ERROR;
    }
}

void DMA1_Stream3_IRQHandler(void) {
    if (READ_BIT(DMA1->LISR, DMA_LISR_TCIF3)) {
        SET_BIT(DMA1->LIFCR, DMA_LIFCR_CTCIF3);
        DAC_TransferCompleteDmaInterrupt(DAC_CH2);
    }

    if (READ_BIT(DMA1->LISR, DMA_LISR_TEIF3)) {
        SET_BIT(DMA1->LIFCR, DMA_LIFCR_CTEIF3);
        error = DMA_ERROR_DAC_INTERRUPT_TRANSFER_ERROR;
    }
}
