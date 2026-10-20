/**
 * @file dsp.c
 * @brief Internal implementation of the DSP module
 *
 * This module provides:
 * - A bundled subset of the ARM CMSIS-DSP library (floating point FIR filtering)
 * - A SensEdu wrapper that hides the CMSIS block processing loop
 */

#include "dsp.h"

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

// Global error container
static DSP_ERROR error = DSP_ERROR_NO_ERRORS;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static void assign_error(DSP_ERROR new_error);
static DSP_ERROR check_fir_settings(SensEdu_DSP_FIR* filter, SensEdu_DSP_FIR_Settings* settings);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

void SensEdu_DSP_FIR_Init(SensEdu_DSP_FIR* filter, SensEdu_DSP_FIR_Settings* settings) {
    DSP_ERROR check = check_fir_settings(filter, settings);
    assign_error(check);
    if (check != DSP_ERROR_NO_ERRORS) {
        // A failed init must never leave a previously configured filter usable
        if (filter != 0) {
            filter->is_init = 0;
        }
        return;
    }

    // also clears the state buffer, so the filter history starts at zero
    arm_fir_init_f32(&filter->instance, settings->tap_num, settings->taps, 
        settings->state_buf, settings->block_size);

    filter->block_size = settings->block_size;
    filter->is_init = 1;
}

// Applied the filter to the `length` samples. Input and output buffers must not overlap.
// The filter history carries over between calls, so a stream can be fed in pieces.
void SensEdu_DSP_FIR_Apply(SensEdu_DSP_FIR* filter, const float* input, float* output,
    uint32_t length) {

    if (filter == 0 || filter->is_init == 0) {
        assign_error(DSP_ERROR_FIR_NOT_INITIALIZED);
        return;
    }
    if (input == 0 || output == 0) {
        assign_error(DSP_ERROR_FIR_NULL_DATA);
        return;
    }

    for (uint32_t i = 0; i < length; i += filter->block_size) {
        // handle the last block, which is usually shorter than the configured block size
        uint32_t block_size = length - i;
        if (block_size > filter->block_size) {
            block_size = filter->block_size;
        }
        arm_fir_f32(&filter->instance, &input[i], &output[i], block_size);
    }
}

DSP_ERROR DSP_GetError(void) {
    return error;
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */

static void assign_error(DSP_ERROR new_error) {
    if (new_error != DSP_ERROR_NO_ERRORS) {
        error = new_error;
    }
}

static DSP_ERROR check_fir_settings(SensEdu_DSP_FIR* filter, SensEdu_DSP_FIR_Settings* settings) {
    if (settings == 0) {
        return DSP_ERROR_NULL_INPUT_SETTINGS;
    }
    if (filter == 0) {
        return DSP_ERROR_FIR_NULL_FILTER;
    }
    if (settings->taps == 0) {
        return DSP_ERROR_FIR_NULL_TAPS;
    }
    if (settings->state_buf == 0) {
        return DSP_ERROR_FIR_NULL_STATE_BUFFER;
    }
    if (settings->tap_num == 0) {
        return DSP_ERROR_FIR_BAD_TAP_NUM;
    }
    if (settings->block_size == 0) {
        return DSP_ERROR_FIR_BAD_BLOCK_SIZE;
    }
    if (settings->state_buf_size < SENSEDU_DSP_FIR_STATE_SIZE(settings->tap_num, settings->block_size)) {
        return DSP_ERROR_FIR_SMALL_STATE_BUFFER;
    }

    return DSP_ERROR_NO_ERRORS;
}
