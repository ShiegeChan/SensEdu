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
* ***Central Speaker***: connected to the first channel (channel 1) on `DAC0` pin 
* ***Bottom Speaker*** connected to the second channel (channel 2) on `DAC1` pin

The SensEdu library has advanced features accounting for ease of use and code efficiency. To specify the data to be sent, a lookup table (LUT) is used. Both channels of the DAC module are available. Moreover, there are 3 modes in which a waveform can be sent to the module: 

1. *burst mode* - sending LUT values for a specified number of times
2. *single mode* - sending LUT values once (or single burst mode)
3. *continuous mode* - sending LUT values continuously 

Each of the methods are usefull for different applications. 

## Structs

### SensEdu_DAC_Settings

DAC configuration structure. 

```c
typedef struct {
    DAC_Channel* dac_channel;               
    uint32_t sampling_freq;
    uint16_t* mem_address;                  
    uint16_t mem_size;                      
    SENSEDU_DAC_MODE wave_mode;
    uint16_t burst_num;                    
} SensEdu_DAC_Settings;
```

#### Fields
{: .no_toc}
* `dac_channel`: Selects the DAC channel (channel 1 or channel 2)
* `sampling_freq`: Specified DAC sampling frequency. Maximum value that can be set is 60 GHz. 
* `mem_address`: Address of the array's first element written to DAC
* `mem_size`: Number of array elements
* `wave_mode`: 
    * `SENSEDU_DAC_MODE_CONTINUOUS_WAVE` 
    * `SENSEDU_DAC_MODE_SINGLE_WAVE`
    * `SENSEDU_DAC_MODE_BURST_WAVE`
* `burst_num`: Number of instances of specified LUT. 

#### Notes
{: .no_toc}
`burst_num` is not zero only for `SENSEDU_DAC_MODE_BURST_WAVE` mode. 


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
{: .no_toc}
* Additionally, initializes associated DMA and timer in ---- modes respectively. 

### SensEdu_DAC_Enable
Powers on the DAC module. 
```c
void SensEdu_DAC_Enable(DAC_Channel* dac_channel);
```
#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance.


### SensEdu_DAC_Disable
Deactivates the DAC module. 

```c
void SensEdu_DAC_Disable(DAC_Channel* dac_channel);
```
#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance. 


### SensEdu_DAC_GetBurstCompleteFlag

Returns the burst status flag of the DAC channel. If the sending mode is in "Burst Mode", the burst_complete flag will be set to `true`. 

```c
uint8_t SensEdu_DAC_GetBurstCompleteFlag(DAC_Channel* dac_channel);
```

#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance. 


### SensEdu_DAC_ClearBurstCompleteFlag

Resets the burst status flag of the DAC channel to its default value `false`. 

```c
void SensEdu_DAC_ClearBurstCompleteFlag(DAC_Channel* dac_channel);
```

#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance. 

### DAC_GetError
Returns a variable of user-defined type DAC_ERROR which specifies a particular error that the DAC module can be in. 
```c
DAC_ERROR DAC_GetError(void);
```

### DAC_WriteDataManually
Writing specified data to the different DAC module register depending on which channel is 
selected. 

```c
void DAC_WriteDataManually(DAC_Channel* dac_channel, uint16_t data)
```
{: .note }
For writing data to one of the channels, right-aligned data registers are used for both channels. 

#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance. 
* `data`: Data to be written to the selected DAC Channel. 

### DAC_ReadCurrentOutputData

Reads the DAC module register for selected DAC channel. 

```c
uint16_t DAC_ReadCurrentOutputData(DAC_Channel* dac_channel);
```

#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance.

## Examples

Each example uses a LUT with specified (16-bit) values and size. An example of defining a sine wave of 64 samples is shown in the following code snippet

```c
const SENSEDU_DAC_BUFFER(buffer_name, buffer_size) = {...};
```
where the first parameter of *SENSEDU_DAC_BUFFER* is the user-defined ***name*** to be used in the program code while the second parameter is the ***size*** of the LUT. 

{: .note}
User can specify LUT size to be any positive integer. However, the real size of the DAC buffer has to be an integer devisible by integers raised by power of 2. See examples for more clarification. 

### Send_DAC_Variable_Wave

Example of modifying the predefined LUT during the program execution. 

1. Define the initial LUT 
2. Configure DAC channel
3. Initialize and enable DAC channel
4. Modify LUT in a loop while making sure it does not go out of bound
5. Check for errors


```c
#include <SensEdu.h>

static uint32_t lib_error = 0;
static uint8_t increment_flag = 1; // run time modification flag

const size_t lut_size = 4;
static SENSEDU_DAC_BUFFER(lut, lut_size) = {
    0x0000,0x0001,0x0002,0x0003
};

void setup() {
    Serial.begin(115200);
    SensEdu_DAC_Settings dac_settings = {DAC_CH1, 64000*16, (uint16_t*)lut, lut_size, 
        SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0};

    SensEdu_DAC_Init(&dac_settings);
    SensEdu_DAC_Enable(DAC_CH1);

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    Serial.println("Setup is successful.");
}

void loop() {
    // modify lut
    for (uint16_t i = 0; i < lut_size; i++) {
        if (increment_flag) {
            lut[i]++;
        } else {
            lut[i]--;
        }
    }

    // out of bounds checks
    if (lut[0] == 0x0000) {
        increment_flag = 1;
    }
    if (lut[lut_size-1] == 0x0FFF) {
        increment_flag = 0;
    }
    
    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}

```

### Send_DAC_Single_Sine

Example of sending a single instance of a predefined LUT with sine waveform. 

```c
#include <SensEdu.h>

uint32_t lib_error = 0;

// DAC transfered symbols
const size_t sine_lut_size = 64;
const SENSEDU_DAC_BUFFER(sine_lut, sine_lut_size) = {0x0000,0x000a,0x0027,0x0058,0x009c,0x00f2,0x0159,0x01d1,0x0258,0x02ed,0x038e,0x043a,0x04f0,0x05ad,0x0670,0x0737,
	0x0800,0x08c8,0x098f,0x0a52,0x0b0f,0x0bc5,0x0c71,0x0d12,0x0da7,0x0e2e,0x0ea6,0x0f0d,0x0f63,0x0fa7,0x0fd8,0x0ff5,
	0x0fff,0x0ff5,0x0fd8,0x0fa7,0x0f63,0x0f0d,0x0ea6,0x0e2e,0x0da7,0x0d12,0x0c71,0x0bc5,0x0b0f,0x0a52,0x098f,0x08c8,
	0x0800,0x0737,0x0670,0x05ad,0x04f0,0x043a,0x038e,0x02ed,0x0258,0x01d1,0x0159,0x00f2,0x009c,0x0058,0x0027,0x000a};

void setup() {
    Serial.begin(115200);
    SensEdu_DAC_Settings dac_settings = {DAC_CH1, 32000*64, (uint16_t*)sine_lut, sine_lut_size, 
        SENSEDU_DAC_MODE_SINGLE_WAVE, 0};

    SensEdu_DAC_Init(&dac_settings);
    SensEdu_DAC_Enable(DAC_CH1);

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
    Serial.println("Setup is successful.");
}

void loop() {
    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}
```

{: .note}
To send a continuous sine wave, the only modification would be DAC mode parameter, choosing SENSEDU_DAC_MODE_CONTINUOUS_WAVE. Similarly, to send a burst wave, choose DAC mode to be SENSEDU_DAC_MODE_BURST_WAVE with specified number of bursts as the last parameter of dac_settings. 

## Developer Notes

### DMA Streams

Each DAC channel occupies one DMA Stream:
* **Channel 1**: DMA1_Stream2
* **Channel 2**: DMA1_Stream3

{: .warning }
Avoid reusing occupied DMA streams. Refer to [STM32H747 Reference Manual] to find free available streams.

info about internal things, like taken streams, channels and etc.
if you want link, include it like this: [link_name]. and link itself at the bottom

[link_name]: https:://link
[STM32H747 Reference Manual]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf