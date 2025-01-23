#ifndef __DMA_H__
#define __DMA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "SensEdu.h"

typedef enum {
    DMA_ERROR_NO_ERRORS = 0x00,
    DMA_ERROR_ENABLED_BEFORE_INIT = 0x01,
    DMA_ERROR_INTERRUPTS_NOT_CLEARED = 0x02,
    DMA_ERROR_ADC_INTERRUPT_TRANSFER_ERROR = 0x03,
    DMA_ERROR_DAC_INTERRUPT_TRANSFER_ERROR = 0x04,
    DMA_ERROR_MEMORY_WRONG_SIZE = 0x05
} DMA_ERROR;

DMA_ERROR DMA_GetError(void);
uint8_t DMA_GetTransferStatus(void);
void DMA_ClearTransferStatus(void);
void DMA_ADCInitPeriph(uint16_t* mem_address, const uint16_t mem_size);
void DMA_ADCEnablePeriph(uint16_t* mem_address, const uint16_t mem_size);
void DMA_ADCDisablePeriph(void);
void DMA_DACInitPeriph(uint16_t* mem_address, const uint16_t mem_size);
void DMA_DACEnablePeriph(uint16_t* mem_address, const uint16_t mem_size);
void DMA_DACDisablePeriph(void);


#ifdef __cplusplus
}
#endif

#endif // __DMA_H__