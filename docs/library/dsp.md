---
title: DSP
layout: default
parent: Library
math: mathjax
nav_order: 5
---

# DSP Module
{: .fs-8 .fw-500 .no_toc}
---

Digital Signal Processing (DSP) utilities directly on MCU. Currently, provides floating point FIR filtering, commonly used to isolate the transducer band and reject the DC offset together with out-of-band noise before further analysis.
{: .fw-500}

- TOC
{:toc}

{: .NOTE}
Unlike the other library sections, DSP is not a hardware peripheral. It is a pure software module that runs on the Cortex-M7 core.

## Overview

Internally the module is built on a bundled subset of the [Arm CMSIS-DSP] library, which performs the filtering in a single precision floating point on the Cortex-M7 FPU. The relevant sources are shipped inside SensEdu (`\src\cmsis\`). Refer to [Developer Notes]({% link library/dsp.md %}#developer-notes) for details.

A FIR (Finite Impulse Response) filter computes each output sample as a weighted sum of the current and previous input samples:

$$
\begin{equation}
y[n] = \sum_{k=0}^{N-1} h[k] \cdot x[n-k]
\end{equation}
$$

In this formula, $$h[k]$$ are the $$N$$ filter coefficients (taps) and $$x[n]$$ is the input signal. The taps define the filter response and are designed externally, for example with the MATLAB [Filter Designer](https://www.mathworks.com/help/signal/ref/filterdesigner-app.html).

## Errors

The main DSP error code prefix is `0x60xx`. See how to display errors in your Arduino sketch [here]({% link library/index.md %}#error-handling).

An overview of possible errors for DSP:

* `0x6000`: No errors
* `0x6001`: Input settings are `null`
* `0x6002`: Passed filter instance is `null`
* `0x6003`: Filter coefficients (`taps`) are `null`
* `0x6004`: State buffer is `null`
* `0x6005`: Input or output data pointer is `null`
* `0x6006`: Invalid `tap_num`, must be at least 1
* `0x6007`: Invalid `block_size`, must be at least 1
* `0x6008`: State buffer is too small, use `SENSEDU_DSP_FIR_STATE_SIZE()` to size it
* `0x6009`: Filter used before `SensEdu_DSP_FIR_Init()` was called

## Macros

### SENSEDU_DSP_FIR_STATE_SIZE

Returns the required length of the filter state buffer, in samples.

```c
#define SENSEDU_DSP_FIR_STATE_SIZE(tap_num, block_size) ((tap_num) + (block_size) - 1)
```

#### Parameters
{: .no_toc}
* `tap_num`: Number of filter coefficients
* `block_size`: Number of samples processed per internal filter call

#### Notes
{: .no_toc}
* The filter has to remember the previous $$N-1$$ samples to compute the next block, hence the $$\text{tap_num} + \text{block_size} - 1$$ length.
* Use it to fill the `state_buf_size` field of [SensEdu_DSP_FIR_Settings]({% link library/dsp.md %}#sensedu_dsp_fir_settings).

### SENSEDU_DSP_FIR_STATE_BUFFER

Declares a correctly sized state buffer for a FIR filter.

```c
#define SENSEDU_DSP_FIR_STATE_BUFFER(name, tap_num, block_size) \
    float name[SENSEDU_DSP_FIR_STATE_SIZE(tap_num, block_size)]
```

#### Parameters
{: .no_toc}
* `name`: User-defined buffer name to be used in the code
* `tap_num`: Number of filter coefficients
* `block_size`: Number of samples processed per internal filter call

#### Notes
{: .no_toc}
* The buffer is a memory owned by the filter. Writing to it between calls corrupts the filter history and produces wrong output.
* Each filter instance should have its own buffer. Two filters sharing one buffer overwrite each other's history.

{: .WARNING}
Always use `SENSEDU_DSP_FIR_STATE_BUFFER` to declare the state buffer. The library does not measure the array you pass, it only compares the `state_buf_size` value you pass against the required length. A value that overstates the real array lets the filter write past the end of it.

## Structs

### SensEdu_DSP_FIR_Settings

FIR filter configuration structure.

```c
typedef struct {
    const float* taps;
    uint16_t tap_num;
    float* state_buf;
    uint16_t state_buf_size;
    uint16_t block_size;
} SensEdu_DSP_FIR_Settings;
```

#### Fields
{: .no_toc}
* `taps`: Pointer to the filter coefficients array
* `tap_num`: Number of filter coefficients
* `state_buf`: Pointer to the state buffer declared with `SENSEDU_DSP_FIR_STATE_BUFFER`
* `state_buf_size`: State buffer length in samples, use `SENSEDU_DSP_FIR_STATE_SIZE()`
* `block_size`: Number of samples processed per internal filter call

#### Notes
{: .no_toc}
* `block_size` does not limit how many samples you may filter at once. It only controls the internal chunking, see [Block Processing]({% link library/dsp.md %}#block-processing).
* A `block_size` of 32 is a reasonable default.

### SensEdu_DSP_FIR

FIR filter instance. Holds the filter configuration and its history between calls.

```c
typedef struct {
    arm_fir_instance_f32 instance;
    uint16_t block_size;
    uint8_t is_init;
} SensEdu_DSP_FIR;
```

#### Notes
{: .no_toc}
* Treat this struct as opaque. You only declare it and pass its address to the DSP functions, never read or write the fields yourself.
* You declare the instance rather than the library, so you can create as many independent filters as you need, for example one per channel or one per frequency band. Each instance requires its own state buffer.
* Reusing a single instance for several signals is also valid and saves memory. In that case call `SensEdu_DSP_FIR_Init()` again before each signal, otherwise the history of the previous one leaks into the first `tap_num - 1` output samples.

## Functions

### SensEdu_DSP_FIR_Init

Initializes a FIR filter with the specified settings.

```c
void SensEdu_DSP_FIR_Init(SensEdu_DSP_FIR* filter, SensEdu_DSP_FIR_Settings* settings);
```

#### Parameters
{: .no_toc}
* `filter`: FIR filter instance
* `settings`: FIR filter configuration structure

#### Notes
{: .no_toc}
* Validates the settings and reports any problem through the [error codes]({% link library/dsp.md %}#errors).
* Clears the state buffer, so the filter history starts at zero.
* Must be called before any call to `SensEdu_DSP_FIR_Apply()`.
* Call it again before every independent signal, such as a new capture or a different channel. Otherwise, the previous signal's tail leaks into the first `tap_num - 1` output samples. Keep a single initialization only when feeding one continuous stream in pieces.

### SensEdu_DSP_FIR_Apply

Filters a block of samples.

```c
void SensEdu_DSP_FIR_Apply(SensEdu_DSP_FIR* filter, const float* input, float* output,
    uint32_t length);
```

#### Parameters
{: .no_toc}
* `filter`: Initialized FIR filter instance
* `input`: Pointer to the input samples
* `output`: Pointer to the output buffer, at least `length` samples long
* `length`: Number of samples to filter

#### Notes
{: .no_toc}
* Handles the internal chunking into `block_size` pieces, including a shorter final block.
* The filter history carries over between calls, so a continuous stream can be fed in pieces.

{: .WARNING}
The `input` and `output` buffers must not overlap. The function still needs the original input samples while it writes the result. Use a separate output buffer and copy the result back if required.

## Examples

Filtering is only meaningful together with acquired data, so the DSP example builds on an ultrasonic measurement.

### Basic_UltraSound_Filtered

This is [Basic_UltraSound]({% link library/others.md %}#basic_ultrasound) with a 32 kHz bandpass added. It emits a sine burst, records one microphone, filters the echo, and sends both the raw and the filtered signal so they can be plotted against each other.

1. Define the filter coefficients, here in a separate `FilterTaps.h` file
2. Declare the state buffer with `SENSEDU_DSP_FIR_STATE_BUFFER`
3. Declare the filter instance and fill the `SensEdu_DSP_FIR_Settings` struct
4. Initialize the filter with `SensEdu_DSP_FIR_Init()`
5. Rescale the raw ADC counts to floats
6. Call `SensEdu_DSP_FIR_Apply()` on the rescaled samples

```c
// FilterTaps.h
#define FILTER_TAP_NUM 64

static float filter_taps[FILTER_TAP_NUM] = {
    -0.00091879f, -0.01806528f, -0.00259733f, 0.00032985f,
    ...
};
```

```c
#include "SensEdu.h"
#include "FilterTaps.h"

#define FILTER_BLOCK_LENGTH 32

static SENSEDU_DSP_FIR_STATE_BUFFER(fir_state_buffer, FILTER_TAP_NUM, FILTER_BLOCK_LENGTH);
SensEdu_DSP_FIR fir_filt;

SensEdu_DSP_FIR_Settings fir_settings = {
    .taps = filter_taps,
    .tap_num = FILTER_TAP_NUM,
    .state_buf = fir_state_buffer,
    .state_buf_size = SENSEDU_DSP_FIR_STATE_SIZE(FILTER_TAP_NUM, FILTER_BLOCK_LENGTH),
    .block_size = FILTER_BLOCK_LENGTH
};

static float rescaled_data[mic_data_size];
static float filtered_data[mic_data_size];

void setup() {
    SensEdu_DSP_FIR_Init(&fir_filt, &fir_settings);
}
```

Normalizing from `0:65535` to `-1:1`:

```c
void rescale_adc_wave(float* rescaled_wave, uint16_t* adc_wave, const uint16_t data_length) {
    for (uint16_t i = 0; i < data_length; i++) {
        rescaled_wave[i] = (2.0f * adc_wave[i]) / 65535.0f - 1.0f;
    }
}
```

Each triggered capture is an independent measurement rather than a continuation of the previous one, so the filter is re-initialized every time to clear the history:

```c
void filter_32kHz_wave(float* input, float* output, const uint16_t data_length) {
    SensEdu_DSP_FIR_Init(&fir_filt, &fir_settings);
    SensEdu_DSP_FIR_Apply(&fir_filt, input, output, data_length);
}
```

{: .NOTE}
The taps shipped with the example peak at $$\approx32.8~\text{kHz}$$ at the $$250~\text{kHz}$$ sampling rate used here, right on the transducer resonance, with a $$-3~\text{dB}$$ band of $$28.7-36.9~\text{kHz}$$. They are scaled for unity gain at the peak, so the filtered signal keeps the same amplitude scale as the input. Redesign them if you change the sampling rate significantly.

The figure below shows both traces on the same plot. The raw signal carries out-of-band noise, while the filtered one keeps only the transducer band, which makes the echo easier to locate and process further.

<img src="{{site.baseurl}}/assets/images/basic_ultrasound_filtered.png" alt="drawing" width="500"/>

## Developer Notes

### Bundled CMSIS-DSP

SensEdu ships a subset of [Arm CMSIS-DSP] in `\src\cmsis\`, limited to the floating point FIR implementation and the headers it depends on:

```
arm_compiler_specific.h       arm_math_types.h             none.h
arm_fir_f32.c                 basic_math_functions.h       support_functions.h
arm_fir_init_f32.c            fast_math_functions.h        utils.h
arm_math_memory.h             filtering_functions.h
```

The `arm_*` functions and types are an internal implementation. Use the `SensEdu_DSP_*` API instead.

{: .NOTE}
The bundled files are Copyright (c) 2010-2021 Arm Limited and licensed under the [Apache License 2.0], which is compatible with the GPL-3.0 license of SensEdu. Attribution and the list of modifications are kept in `NOTICE.txt` next to the sources.

### Block Processing

State buffer has to hold the $$\text{tap_num} - 1$$ history samples followed by exactly `block_size` new samples in contiguous memory. This is why the buffer length is derived from both values.

`SensEdu_DSP_FIR_Apply()` walks the input in `block_size` chunks and shortens the last one if `length` is not a multiple of `block_size`.

This is why user code never needs its own chunking loop, and why `block_size` is only a tuning parameter rather than a limit on how much data can be filtered.


[Arm CMSIS-DSP]: https://github.com/ARM-software/CMSIS-DSP
[Apache License 2.0]: https://www.apache.org/licenses/LICENSE-2.0
