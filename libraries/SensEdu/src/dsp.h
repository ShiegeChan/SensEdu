/**
 * @file dsp.h
 * @brief Internal API for the DSP module
 *
 * This module provides:
 * - A bundled subset of the ARM CMSIS-DSP library (floating point FIR filtering)
 * - A SensEdu wrapper that hides the CMSIS block processing loop
 */

#ifndef __DSP_H__
#define __DSP_H__

#include "libs.h"
#include "cmsis/filtering_functions.h"

#ifdef __cplusplus
extern "C" {
#endif

// CMSIS keeps the previous (tap_num - 1) samples plus the current block in the state buffer
#define SENSEDU_DSP_FIR_STATE_SIZE(tap_num, block_size) \
    ((tap_num) + (block_size) - 1)

#define SENSEDU_DSP_FIR_STATE_BUFFER(name, tap_num, block_size) \
    float name[SENSEDU_DSP_FIR_STATE_SIZE(tap_num, block_size)]

typedef enum {
    DSP_ERROR_NO_ERRORS = 0x00,
    DSP_ERROR_NULL_INPUT_SETTINGS = 0x01,
    DSP_ERROR_FIR_NULL_FILTER = 0x02,
    DSP_ERROR_FIR_NULL_TAPS = 0x03,
    DSP_ERROR_FIR_NULL_STATE_BUFFER = 0x04,
    DSP_ERROR_FIR_NULL_DATA = 0x05,
    DSP_ERROR_FIR_BAD_TAP_NUM = 0x06,
    DSP_ERROR_FIR_BAD_BLOCK_SIZE = 0x07,
    DSP_ERROR_FIR_SMALL_STATE_BUFFER = 0x08,
    DSP_ERROR_FIR_NOT_INITIALIZED = 0x09
} DSP_ERROR;

typedef struct {
    const float* taps;          // Filter coefficients
    uint16_t tap_num;           // Number of coefficients
    float* state_buf;           // Scratch buffer, filter history is kept here between calls
    uint16_t state_buf_size;    // Length in samples, use SENSEDU_DSP_FIR_STATE_SIZE()
    uint16_t block_size;        // Samples processed per CMSIS call
} SensEdu_DSP_FIR_Settings;

typedef struct {
    arm_fir_instance_f32 instance;
    uint16_t block_size;
    uint8_t is_init;
} SensEdu_DSP_FIR;

void SensEdu_DSP_FIR_Init(SensEdu_DSP_FIR* filter, SensEdu_DSP_FIR_Settings* settings);
void SensEdu_DSP_FIR_Apply(SensEdu_DSP_FIR* filter, const float* input, float* output,
    uint32_t length);

DSP_ERROR DSP_GetError(void);


#ifdef __cplusplus
}
#endif

#endif // __DSP_H__
