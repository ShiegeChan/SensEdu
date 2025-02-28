---
title: ADC
layout: default
parent: Library
math: mathjax
nav_order: 2
---

# ADC Peripheral
{: .fs-8 .fw-500 .no_toc}
---

Analog-to-Digital Converters (ADCs) translate analog signals (e.g., from sensors or microphones) into digital form, readable by CPU. 
{: .fw-500}

The STM32H747 features three 16-bit ADCs with up to 20 multiplexed channels each.

- TOC
{:toc}

## Structs

### SensEdu_ADC_Settings
ADC configuration structure.

```c
typedef struct {
    ADC_TypeDef* adc;
    uint8_t* pins;
    uint8_t pin_num;

    SENSEDU_ADC_CONVMODE conv_mode;
    uint32_t sampling_freq;
    
    SENSEDU_ADC_DMA dma_mode;
    uint16_t* mem_address;
    uint16_t mem_size;
} SensEdu_ADC_Settings;
```

#### Fields
{: .no_toc}
* `adc`: Selects the ADC peripheral (`ADC1`, `ADC2` or `ADC3`)
* `pins`: Array of Arduino-labeled analog pins (e.g., {`A0`, `A3`, `A7`}). The ADC will sequentially sample these pins
* `pin_num`: `pins` array length
* `conv_mode`:
  * `SENSEDU_ADC_MODE_ONE_SHOT`: Single conversion on demand
  * `SENSEDU_ADC_MODE_CONT`: Continuous conversions
  * `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED`: Timer-driven continuous conversions, which enables stable sampling frequency
* `sampling_freq`: Specified sampling frequency for `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED` mode. Up to ~500kS/sec
* `dma_mode`: Specifies if ADC values are manually polled with CPU or automatically transferred into memory with DMA:
  * `SENSEDU_ADC_DMA_CONNECT`: Attach DMA
  * `SENSEDU_ADC_DMA_DISCONNECT`: CPU polling
* `mem_address`: DMA buffer address in memory (first element of the array)
* `mem_size`: DMA buffer size

#### Notes
{: .no_toc}

Be aware of which pins you can use with selected ADC. Table below shows ADC connections. For example, you can't access `ADC3` with pin `A7`, cause it is only connected to `ADC1`. 

* `ADCx_INPy`:
  * `x`: Connected ADCs (e.g., `ADC12_INP4` - `ADC1` and `ADC2`)
  * `y`: Channel index 

| Arduino Pin | STM32 GPIO | Available ADCs |
|:------------|:-----------|:---------------|
| A0          | PC4        | ADC12_INP4     |
| A1          | PC5        | ADC12_INP8     |
| A2          | PB0        | ADC12_INP9     |
| A3          | PB1        | ADC12_INP5     |
| A4          | PC3        | ADC12_INP13    |
| A5          | PC2        | ADC123_INP12   |
| A6          | PC0        | ADC123_INP10   |
| A7          | PA0        | ADC1_INP16     |
| A8          | PC2_C      | ADC3_INP0      |
| A9          | PC3_C      | ADC3_INP1      |
| A10         | PA1_C      | ADC12_INP1     |
| A11         | PA0_C      | ADC12_INP0     |


## Functions

### SensEdu_ADC_Init
Configures ADC clock and initializes peripheral with specified settings (channels, sampling frequency, etc.).

```c
void SensEdu_ADC_Init(SensEdu_ADC_Settings* adc_settings);
```

#### Parameters
{: .no_toc}
* `adc_settings`: ADC configuration structure

#### Notes
{: .no_toc}
* Initializes associated DMA and timer in `SENSEDU_ADC_DMA_CONNECT` and `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED` modes respectively.

{: .warning}
Be careful to initialize each required ADC before enabling. Certain configuration is shared between multiple ADCs, which could be edited only if related ADCs are disabled.

```c
// ERROR
SensEdu_ADC_Init(ADC1_Settings);
SensEdu_ADC_Enable(ADC1);
SensEdu_ADC_Init(ADC2_Settings);
SensEdu_ADC_Enable(ADC2);

// CORRECT
SensEdu_ADC_Init(ADC1_Settings);
SensEdu_ADC_Init(ADC2_Settings);
SensEdu_ADC_Enable(ADC1);
SensEdu_ADC_Enable(ADC2);
```


### SensEdu_ADC_Enable
Powers on the ADC.

```c
void SensEdu_ADC_Enable(ADC_TypeDef* ADC);
```

#### Parameters
{: .no_toc}
* `ADC`: ADC Instance (`ADC1`, `ADC2` or `ADC3`)

#### Notes
{: .no_toc}
* Enables sampling frequency timer in `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED` mode.
* **Don't confuse with `SensEdu_ADC_Start`.** `Enable` turns ADC on, but `Start` is used to trigger conversions.


### SensEdu_ADC_Disable
Deactivates the ADC.

```c
void SensEdu_ADC_Disable(ADC_TypeDef* ADC);
```

#### Parameters
{: .no_toc}
* `ADC`: ADC Instance (`ADC1`, `ADC2` or `ADC3`)

#### Notes
{: .no_toc}
* Disables associated DMA in `SENSEDU_ADC_DMA_CONNECT` mode.


### SensEdu_ADC_Start
Triggers ADC conversions.

```c
void SensEdu_ADC_Start(ADC_TypeDef* ADC);
```

#### Parameters
{: .no_toc}
* `ADC`: ADC Instance (`ADC1`, `ADC2` or `ADC3`)

#### Notes
{: .no_toc}
* Enables associated DMA in `SENSEDU_ADC_DMA_CONNECT` mode.


### SensEdu_ADC_GetTransferStatus
Returns current DMA transfer status (`dma_complete` flag)

```c
uint8_t SensEdu_ADC_GetTransferStatus(ADC_TypeDef* adc);
```

#### Parameters
{: .no_toc}
* `ADC`: ADC Instance (`ADC1`, `ADC2` or `ADC3`)

#### Returns
{: .no_toc}
* `dma_complete` flag: `HIGH` indicated that DMA controller finished memory transfer

#### Notes
{: .no_toc}
* `dma_complete` flag is automatically cleared by calling `SensEdu_ADC_Start()`

{: .warning}
Avoid performing any actions without acknowledging this flag. It ensures that the data was completely transferred.


### SensEdu_ADC_ReadConversion
Manually read a single ADC conversion (alternative to DMA).

```c
uint16_t SensEdu_ADC_ReadConversion(ADC_TypeDef* ADC)
```

#### Parameters
{: .no_toc}
* `ADC`: ADC Instance (`ADC1`, `ADC2` or `ADC3`)

#### Returns
{: .no_toc}
* 16-bit value from selected channel

#### Notes
{: .no_toc}
* Used for readings using single channel. For multi-channel readings, use `SensEdu_ADC_ReadSequence`.
* Consumes CPU cycles. Prefer DMA to free CPU for other tasks. For example, you can perform complex calculations on ADC values, while requesting the new set of data with DMA.
* Refer to non-DMA examples like `Read_ADC_1CH`.


### SensEdu_ADC_ReadSequence
Manually read a sequence of ADC conversions (alternative to DMA).

```c
uint16_t SensEdu_ADC_ReadSequence(ADC_TypeDef* ADC)
```

#### Parameters
{: .no_toc}
* `ADC`: ADC Instance (`ADC1`, `ADC2` or `ADC3`)

#### Returns
{: .no_toc}
* A pointer to an array of ADC conversion results. Index the array to access values: `[0]`, `[1]`, `[2]`, etc., corresponding to the amount of selected channels (`pins` array in `SensEdu_ADC_Settings`)

#### Notes
{: .no_toc}
* Used for readings using multiple channels. For single-channel readings, use `SensEdu_ADC_ReadConversion`.
* Consumes CPU cycles. Prefer DMA to free CPU for other tasks. For example, you can perform complex calculations on ADC values, while requesting the new set of data with DMA.
* Refer to non-DMA examples like `Read_ADC_3CH`.

{: .warning}
Multi-channel CPU polling currently doesn't work in `SENSEDU_ADC_MODE_ONE_SHOT`. Use `SENSEDU_ADC_MODE_CONT` or `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED`. If you want to look into this and have a try fixing it, refer to [this issue].


### SensEdu_ADC_ShortA4toA9
Shorts pin `A4` to `A9` on Arduino. 

```c
void SensEdu_ADC_ShortA4toA9(void);
```

#### Notes
{: .no_toc}
* **In older board revisions**, microphone #2 is wired to pin `A9` (`PC3_C`), which is routed only to `ADC3`. This conflicts with project requiring x4 microphones, using `ADC1` and `ADC2` with x2 channels per ADC. To solve this problem, `_C` pins could be shorted to their `non_C` counterparts. This way pin `A4` (`PC3`) is bridged to `A9` (`PC3_C`), allowing microphone #2 to be accessed via any ADC, since `PC3` is shared between `ADC1` and `ADC2`. Refer to the table at [settings section]({% link Library/ADC.md %}#notes) for better understanding.


## Examples
Examples are organized incrementally. Each builds on the previous one by introducing only new features or modifications. Refer to earlier examples for core functionality details.
{: .fw-500}

If you want to see complete examples, visit `\examples\` directory or open them via Arduino IDE by navigating to `File → Examples → SensEdu`.

### Read_ADC_1CH

Continuously reads ADC conversions directly via CPU for one selected analog pin.

1. Include SensEdu library
2. Declare ADC instance, pin array and array size corresponding to your channel count for selected ADC
3. Configure ADC Parameters by declaring [`SensEdu_ADC_Settings`]({% link Library/ADC.md %}#sensedu_adc_settings) struct
4. Initialize `SensEdu_ADC_Init()` and power up ADC `SensEdu_ADC_Enable()`
5. Start ADC once with `SensEdu_ADC_Start()` (**once** applies only for continuous mode `SENSEDU_ADC_MODE_CONT`)
6. In a loop, manually read data from a single channel using `SensEdu_ADC_ReadConversion()`. Print results with `Serial`
7. Open Serial Monitor to see results. Try to connect selected pin to GND or 3.3V. The values should vary in a range from 0 to 65535

```c
#include "SensEdu.h"

ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 1;
uint8_t adc_pins[adc_pin_num] = {A0};
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT,
    .sampling_freq = 0,
    
    .dma_mode = SENSEDU_ADC_DMA_DISCONNECT,
    .mem_address = 0x0000,
    .mem_size = 0
};

void setup() {
    Serial.begin(115200);
    
    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
    SensEdu_ADC_Start(adc);
}

void loop() {
    uint16_t data = SensEdu_ADC_ReadConversion(adc);
    Serial.println(data);
}
```

#### Notes
{: .no_toc}
* For the most simple CPU polling configuration there are unused parameters like `.sampling_rate` or `.mem_address`. Such parameters could be set to any value, they are completely ignored.
* ADC values are 16-bit, vary from 0 (0V) to 65535 (3.3V).


### Read_ADC_3CH

Continuously reads sequences of ADC conversions directly via CPU for multiple selected analog pins.

1. Follow base configuration from the [`Read_ADC_1CH`]({% link Library/ADC.md %}#read_adc_1ch) example
2. Expand pin array to include all desired channels. Update array size to match channel count
3. Use `SensEdu_ADC_ReadSequence()` to retrieve a channel sequence array

```c
...
const uint8_t adc_pin_num = 3;
uint8_t adc_pins[adc_pin_num] = {A0, A1, A2};
...
void loop() {
    uint16_t* data = SensEdu_ADC_ReadSequence(adc);
    Serial.println("-------");
    for (uint8_t i = 0; i < adc_pin_num; i++) {
        Serial.print("Value CH");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(data[i]);
    }
}
```

#### Notes
{: .no_toc}
* Compared to single-channel configuration, `SensEdu_ADC_ReadSequence()` returns not a value, but a pointer. Using this pointer, you can access all channels in a sequence with index brackets `[]`.
* ADC conversions are organised in a "package" called **sequence**. They follow exact order defined in `adc_pins` (A0 → A1 → A2 in this example).


### Read_ADC_1CH_TIM

Continuously reads ADC conversions directly via CPU for one selected analog pin with constant sampling rate using timer trigger.

1. Follow base configuration from the [`Read_ADC_1CH`]({% link Library/ADC.md %}#read_adc_1ch) example
2. Update conversion mode `.conv_mode` in `SensEdu_ADC_Settings` to `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED`
3. Specify sampling frequency in Hz via `.sampling_freq`

```c
...
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_DISCONNECT,
    .mem_address = 0x0000,
    .mem_size = 0
};
...
```

#### Notes
{: .no_toc}
* Expect small variations from specified sampling frequency


### Read_ADC_3CH_TIM

Continuously reads sequences of ADC conversions directly via CPU for multiple selected analog pins with constant sampling rate using timer trigger.

1. Follow base multi-channel configuration from the [`Read_ADC_3CH`]({% link Library/ADC.md %}#read_adc_3ch) example
2. Update conversion mode `.conv_mode` in `SensEdu_ADC_Settings` to `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED`
3. Specify sampling frequency in Hz via `.sampling_freq`

```c
...
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_DISCONNECT,
    .mem_address = 0x0000,
    .mem_size = 0
};
...
```

#### Notes
{: .no_toc}
* Expect small variations from specified sampling frequency


### Read_ADC_1CH_DMA

Continuously reads ADC conversions using DMA for a single analog pin, allowing efficient data transfer without CPU intervention.

1. Follow base configuration from the [`Read_ADC_1CH`]({% link Library/ADC.md %}#read_adc_1ch) example
2. Create an array to store ADC results. The array's size (in bytes) must be a multiple of the STM32 cache line size (32 bytes for STM32H747). For a `uint16_t` array, this means the number of elements should be a multiple of 16 (each element is 2 bytes). For example, sizes like 16, 32, 64, etc.
3. Align the array to the cache line using `__attribute__((aligned(__SCB_DCACHE_LINE_SIZE)))`
4. In `SensEdu_ADC_Settings`, set `.dma_mode` to `SENSEDU_ADC_DMA_CONNECT`
5. Assign `.mem_address` to the array's first element address and `.mem_size` to its length
6. After calling `SensEdu_ADC_Start()`, the ADC fills the buffer with conversions and automatically stops, setting the `dma_complete` flag
7. Check `dma_complete` using `SensEdu_ADC_GetTransferStatus()`. When `true`, read the buffer and perform operations (e.g., print values, compute something)
8. Call `SensEdu_ADC_Start()` again to trigger a new DMA transfer

{: .warning}
In future releases cache alignment will be automated, allowing arbitrary buffer sizes with a command like `SENSEDU_ADC_BUFFER(memory4adc, memory4adc_size);`. You can contribute to this feature [here](https://github.com/ShiegeChan/SensEdu/issues/10).

```c
#include "SensEdu.h"

const uint16_t memory4adc_size = 128;
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t memory4adc[memory4adc_size];

ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 1;
uint8_t adc_pins[adc_pin_num] = {A0};
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT,
    .sampling_freq = 0,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)memory4adc,
    .mem_size = memory4adc_size
};

void setup() {
    Serial.begin(115200);

    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
    SensEdu_ADC_Start(adc);
}

void loop() {
    // do something else when transfer is not yet completed
    if (SensEdu_ADC_GetTransferStatus(adc)) {
        Serial.println("------");
        for (int i = 0; i < memory4adc_size; i++) {
            Serial.print("ADC value ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(memory4adc[i]);
        };

        SensEdu_ADC_Start(adc);
    }
}
```

#### Notes
{: .no_toc}
* `SensEdu_ADC_Start()` resets the `dma_complete` flag automatically.
* Optimize your code to use DMA capability to perform memory transfers in the background. For example, start a new measurement in the middle of calculations, when the whole old dataset is not needed anymore.
* Cache line alignment and cache invalidation are necessary to ensure cache coherence and prevent data corruption. When data is transferred to a cached memory chunk, the CPU may read outdated data from the cache. To avoid this issue, we need to invalidate the affected memory. Invalidation operation is performed for the entire cache line, which is 32 bytes long for the STM32H747.


### Read_ADC_3CH_DMA

Continuously reads ADC conversions using DMA multiple selected analog pins, allowing efficient data transfer without CPU intervention.

1. Follow the DMA configuration from the [`Read_ADC_1CH_DMA`]({% link Library/ADC.md %}#read_adc_1ch_dma) example
2. Expand pin array to include all desired channels. Update array size to match channel count
3. Expand the ADC DMA buffer accordingly to include data for all channels. Ensure that the entire buffer size is still a multiple of the STM32 cache line size (32 bytes for STM32H747)
4. Data is organized in a sequence, following the order defined in pin array (e.g., A0 → A1 → A2 → A0 → A1 → ...)


```c
...
const uint8_t adc_pin_num = 3;
uint8_t adc_pins[adc_pin_num] = {A0, A1, A2}; 

const uint16_t memory4adc_size = 64 * adc_pin_num;
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t memory4adc[memory4adc_size];
...
void loop() {
    // do something else when transfer is not yet completed
    if (SensEdu_ADC_GetTransferStatus(adc)) {
        Serial.println("------");
        for (int i = 0; i < memory4adc_size; i+=3) {
            Serial.print("ADC value ");
            Serial.print(i/3);
            Serial.print("CH1: ");
            Serial.println(memory4adc[i]);

            Serial.print("ADC value ");
            Serial.print(i/3);
            Serial.print("CH2: ");
            Serial.println(memory4adc[i+1]);

            Serial.print("ADC value ");
            Serial.print(i/3);
            Serial.print("CH3: ");
            Serial.println(memory4adc[i+2]);
        }

        SensEdu_ADC_Start(adc);
    };
}
```


### Read_2ADC_3CH_DMA

This example demonstrates the usage of multiple ADCs in DMA mode. Essentially, you follow the same steps as in the [`Read_ADC_3CH_DMA`]({% link Library/ADC.md %}#read_adc_3ch_dma) example, but use separate buffers, configuration structures, and function calls for each ADC.

For example:

```c
SensEdu_ADC_Init(&adc1_settings);
SensEdu_ADC_Enable(ADC1);
SensEdu_ADC_Start(ADC1);

SensEdu_ADC_Init(&adc2_settings);
SensEdu_ADC_Enable(ADC2);
SensEdu_ADC_Start(ADC2);
```

#### Notes
{: .no_toc}
* Ensure there is no pin overlap between different ADCs (e.g., do not include pin A0 in the arrays for both ADC1 and ADC2).


## Developer Notes

### DMA Streams

Each ADC occupies one DMA stream:
* **ADC1**: DMA1_Stream6
* **ADC2**: DMA1_Stream5
* **ADC3**: DMA1_Stream7

{: .warning }
Avoid reusing occupied DMA streams. Refer to [STM32H747 Reference Manual] to find free available streams.


### Conversion Time

You can calculate the needed time for each conversion $$(T_{CONV})$$ with this formula:

$$T_{CONV} = T_{SMPL} + T_{SAR}$$

* $$T_{SMPL}$$: Configured sampling time
* $$T_{SAR}$$: Successive approximation time depending on data resolution

In SensEdu, $$T_{SMPL}$$ is configured to $$2.5$$ ADC clock cycles, which correpsonds to bits `SMP[2:0] = 0b001` in the **ADC_SMPR1** and **ADC_SMPR2** registers.

ADC conversions are fixed to 16-bit resolution, so $$T_{SAR}$$ is constant and equals to $$8.5$$ ADC clock cycles.

The ADC clock is routed from the PLL2 clock and set to $$25\text{MHz}$$ for each individual ADC, which gives us:

$$T_{CONV} = (2.5 \text{ cycles} + 8.5 \text{ cycles}) * \frac{1}{f_{\text{adc_ker_ck}}} = 11 \text{ cycles} * \frac{1}{25\text{MHz}} = 440\text{ns}$$

SensEdu is configured to x2 oversampling (basically, averaging), so we require around $$880\text{ns}$$ per one ADC conversion, which theoretically gives us a maximum $$1136\text{kS/sec}$$ sampling rate. However, in reality, this rate could be lower due to various additional delays. We assume a maximum sampling rate of $$500\text{kS/sec}$$, but it needs to be additionally tested and confirmed.


### Initialization

General ADC configuration:

| Register name | Register Field | Value | [Manual Page] | Function |
|:-------|:-------|:--------|:------------------------|
| CR     | BOOST  | 0b10    | 26.4.3 <br> Page: 958   | ADC clock range $$12.5\text{MHz}:25\text{MHz}$$ |
| CFGR   | RES    | 0b000   | 26.6.4 <br> Page: 1047  | 16-bit Resolution
| CFGR   | OVRMOD | 0b1     | 26.4.27 <br> Page: 998  | Overrun mode (overwrite data) |
| SMPRx  | SMPy   | 0b001   | 26.4.13 <br> Page: 972  | Sample Time of 2.5 ADC clock cycles
| CFGR2  | OVSR   | 2U - 1U | 26.4.31 <br> Page: 1009 | x2 Oversampling |
| CFGR2  | ROVSE  | 0b1     | 26.4.31 <br> Page: 1009 | Enable Oversampling |
| CFGR2  | OVSS   | 0b0001  | 26.4.31 <br> Page: 1009 | 1-bit right shift to account for x2 oversampling (averaging) |

DMA vs CPU polling specific:

|:-------|:-------|:--------|:------------------------|
| CFGR   | DMNGT  | 0b01    | 26.4.27 <br> Page: 1000 | DMA is enabled in circular mode |
| CFGR   | DMNGT  | 0b00    | 26.4.27 <br> Page: 1000 | Data is stored only in Data Register (DR) |

Timer triggered mode (sampling rate generation) specific:

|:-------|:--------|:-----------------|:-----------------------|
| CFGR   | EXTEN   | 0b01             | 26.4.19 <br> Page: 977 | Enable trigger on rising edge |
| CFGR   | EXTSEL  | depends on TIMx | 26.4.19 <br> Page: 977 | Code for selected timer that triggers ADC conversions |

Continuous mode specific:

|:------|:-----|:----|:-----------------------|
| CFGR  | CONT | 0b0 | 26.4.14 <br> Page: 973 | Single conversion mode (`SENSEDU_ADC_MODE_ONE_SHOT`)
| CFGR  | CONT | 0b1 | 26.4.15 <br> Page: 973 | Continuous conversion mode (`SENSEDU_ADC_MODE_CONT`)


### Clock Configuration

To configure the ADC clock, it is first necessary to configure the **PLL** (Phase-Locked Loop). The PLL contains frequency multipliers and dividers that enable the generation of different frequencies, which are multiples of the input frequency.

The source frequency for the PLL is the HSE (High Speed External Oscillator), which has a frequency of 16MHz.
{: .fw-500}

The clock is first divided by DIVM2, which is set to 4, resulting in a 4MHz PLL2 input frequency ($$\text{ref2_ck}$$). Additionally, the PLL2RGE field in the PLLCFGR register must be configured according to the selected range for $$\text{ref2_ck}$$. Since we use 4MHz, it is set to the 4:8MHz range.

The clock is then multiplied by DIVN2, which is set to 75, resulting in a **VCO** (Voltage-Controlled Oscillator) frequency of 300MHz. The frequency was selected to fit within the chosen VCO range in the PLL2VCOSEL field of the PLLCFGR register, which is set to the narrow range of 150:420MHz.

Finally, the VCO frequency is divided by DIVP2, which is set to 6, resulting in a 50MHz frequency for the shared ADC bus.

![]({{site.baseurl}}/assets/images/PLL_Clock.png)

The ADC clock is selected to be independent and asynchronous with the AHB clock, named $$\text{adc_ker_ck_input}$$ and derived from PLL2. The clock then passes through a settable prescaler CKMODE in the ADCx_CCR register, which is set to 1 (no clock division). Then, it passes through a fixed /2 prescaler, resulting in a 25MHz frequency ($$F_{\text{adc_ker_ck}}$$) for each individual ADC. This frequency must comply with the maximum ADC clock frequency specified in Table 99 of the [STM32H747 Datasheet].

![]({{site.baseurl}}/assets/images/ADC_Clock.png)


### Cache Coherence

When using the ADC with DMA, you need to be aware of cache coherence problems. Keep in mind that all memory is cached for faster access. The objective of DMA is to bypass the CPU and offload memory transfers to the DMA controller. The issue arises when the CPU reads the transferred data, the processor might read outdated data stored in cache instead of the actual data in memory, as it is not aware of DMA transfers.

To ensure that the CPU reads correct data, you need to **invalidate the cache** before accessing any transferred data. This is accomplished using the internal function `SCB_InvalidateDCache_by_Addr(mem_addr, mem_size)` with the following parameters:
* `mem_addr`: Memory address of the ADC buffer
* `mem_size`: Memory size **in bytes**

This cache invalidation is automatically performed inside the `void DMA_ADCEnable(ADC_TypeDef* adc)` function.

The cache invalidation procedure applies to the entire cache line. Therefore, it is essential to align your ADC buffer to the cache line and ensure its size is a multiple of the cache line size. For the STM32H747, the cache line is 32 bytes long and is defined in the macro `__SCB_DCACHE_LINE_SIZE`. For a `uint16_t` array, this means the number of elements must be a multiple of 16 (each element is 2 bytes). For example. valid sizes include 16, 32, 64, etc. Alignment is achieved using the `__attribute__` directive:

```c
const uint16_t memory4adc_size = 128; // multiple of __SCB_DCACHE_LINE_SIZE/2
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t memory4adc[memory4adc_size];
```


[this issue]: https://github.com/ShiegeChan/SensEdu/issues/8
[STM32H747 Reference Manual]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
[Manual Page]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
[STM32H747 Datasheet]: https://www.st.com/resource/en/datasheet/stm32h747ag.pdf