---
title: ADC
layout: default
parent: Library
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
* `sampling_freq`: specified sampling frequency for `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED` mode. Up to ~500kS/sec
* `dma_mode`: specifies if ADC values are manually polled with CPU or automatically transferred into memory with DMA:
  * `SENSEDU_ADC_DMA_CONNECT`: attach DMA
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
* Additionally, initializes associated DMA and timer in `SENSEDU_ADC_DMA_CONNECT` and `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED` modes respectively.

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
* Additionally, enables sampling frequency timer in `SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED` mode.
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
* Additionally, disables associated DMA in `SENSEDU_ADC_DMA_CONNECT` mode.

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
* Additionally, enables associated DMA in `SENSEDU_ADC_DMA_CONNECT` mode.

### SensEdu_ADC_ReadSingleSequence
it does cool things

```c
uint16_t* SensEdu_ADC_ReadSingleSequence(ADC_TypeDef* ADC);
```

#### Parameters
{: .no_toc}
* `parameter`: explanation

#### Returns
{: .no_toc}

### SensEdu_ADC_ShortA4toA9
it does cool things

```c
void SensEdu_ADC_ShortA4toA9(void);
```

#### Parameters
{: .no_toc}
* `parameter`: explanation


## Examples
Examples are presented in repeatable way with succession complexity. So with each more complex example, explained only what changed and you can always refer to previous onces to understand basic operation.

### Read_ADC_1CH

does cool things

1. do this
2. and the this

{: .warning}
something wrong

```c
// simplified minimal code, maybe even split if too long
```

### Read_ADC_1CH_TIM

does cool things

1. do this
2. and the this

{: .warning}
something wrong

```c
// simplified minimal code, maybe even split if too long
```

### Read_ADC_3CH_TIM

does cool things

1. do this
2. and the this

{: .warning}
something wrong

```c
// simplified minimal code, maybe even split if too long
```

### Read_ADC_1CH_DMA

does cool things

1. do this
2. and the this

{: .warning}
something wrong

```c
// simplified minimal code, maybe even split if too long
```

### Read_ADC_3CH_DMA

does cool things

1. do this
2. and the this

{: .warning}
something wrong

```c
// simplified minimal code, maybe even split if too long
```

### Read_2ADC_3CH_DMA

does cool things

1. do this
2. and then this

{: .warning}
something wrong

```c
// simplified minimal code, maybe even split if too long
```


## Developer Notes

TODO: explain adc taken dma streams, how interrupts work, what exactly initialisation sets
TODO: conversion time calculations
TODO: PLL CONFIGURATION and ADC clock, stm32 screenshots
TODO: explain why we need to short A4 to A9
TODO: explain all structs, why we need adc_data, channels, ADC settings
TODO: explain errors


_С pins could be shorted to their non-_C pins:
CLEAR_BIT(SYSCFG->PMCR, SYSCFG_PMCR_PC3SO);
this shorts PC3_C to PC3
and you could access ADC12_INP13 through PC3_C

[link_name]: https:://link