---
title: DAC
layout: default
parent: Library
nav_order: 3
---

# DAC Peripheral
{: .fs-8 .fw-500 .no_toc}
---

A DAC (Digital-to-Analog Converter) converts a digital signal into an analog waveform, which is then sent to a speaker to produce acoustic waves.
{: .fw-500}

- TOC
{:toc}

The STM32H747 features one DAC module with two available channels:
* **Central Speaker**: connected to the first channel (channel 1) on `DAC0` pin 
* **Bottom Speaker** connected to the second channel (channel 2) on `DAC1` pin

## Structs

## Functions 

### SensEdu_DAC_Init
Configures DAC clock and initializes the peripheral with specified settings (channel, sampling frequency, waveform, etc.) 

```c
void SensEdu_DAC_Init(SensEdu_DAC_Settings* dac_settings);
```
#### Parameters
{: .no_toc}
* `dac_settings`: DAC configuration structure

#### Notes
* Additionally, initializes associated DMA and timer in ---- modes respectively. 

### SensEdu_DAC_Enable
Powers on the DAC module. 
```c
void SensEdu_DAC_Enable(DAC_Channel* dac_channel);
```
#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance


### SensEdu_DAC_Disable
Deactivates the DAC module. 
```c
void SensEdu_DAC_Disable(DAC_Channel* dac_channel);
```
#### Parameters
{: .no_toc}
* `parameter`: explanation


### SensEdu_DAC_GetBurstCompleteFlag
it does cool things

#### Parameters
{: .no_toc}
* `parameter`: explanation


### SensEdu_DAC_ClearBurstCompleteFlag
it does cool things

#### Parameters
{: .no_toc}
* `parameter`: explanation

### DAC_GetError
it does cool things

#### Parameters
{: .no_toc}
* `parameter`: explanation

### DAC_WriteDataManually
it does cool things

#### Parameters
{: .no_toc}
* `parameter`: explanation

### DAC_ReadCurrentOutputData
it does cool things

#### Parameters
{: .no_toc}
* `parameter`: explanation






## Examples

### first example

does cool things

1. do this
2. and the this

{: .warning}
something wrong

```c
// simplified minimal code, maybe even split if too long
```

## Developer Notes

info about internal things, like taken streams, channels and etc.
if you want link, include it like this: [link_name]. and link itself at the bottom

[link_name]: https:://link