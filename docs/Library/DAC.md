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

To specify the data to be sent, a lookup table (LUT) is used. There are 3 available modes in which a waveform can be sent to the peripheral: 

1. *continuous mode* - sending LUT values continuously 
2. *burst mode* - sending LUT values specified number of cycles
3. *single mode* - sending LUT values once (single burst mode)

Each of the methods are useful for different applications.

## Errors

Main DAC error code is `0x40xx`. Find the way to display errors in your Arduino sketch [here]({% link Library/index.md %}#error-handling).

An overview of possible errors for DAC:

* `0x4000`: No Errors
* `0x4001`: DAC was initialized before initialization
* `0x4002`: Passed DAC channel is not either `DAC_CH1` nor `DAC_CH2`
* `0x4003`: Selected sampling frequency is too high. Maximum is around 15MHz
* `0x4004`: Unexpected address or memory size for DMA
* `0x4005`: In `SENSEDU_DAC_MODE_BURST_WAVE` expected `burst_num` is at least 1

An overview of critical errors. They shouldn’t happen in normal user case and indicate some problems in library code:

* `0x40A0`: DMA Underrun interrupt flag was raised: currently selected trigger is driving DAC channel conversion at a frequency higher than the DMA service capability rate (read more in section 27.4.8 of [Reference Manual])

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
* `dac_channel`: Selects the DAC channel (`DAC_CH1` or `DAC_CH2`)
* `sampling_freq`: Specified DAC sampling frequency. Maximum value is around 15 MHz
* `mem_address`: DMA buffer address in memory (first element of the array)
* `mem_size`: DMA buffer size
* `wave_mode`: 
    * `SENSEDU_DAC_MODE_CONTINUOUS_WAVE`: Continuous mode
    * `SENSEDU_DAC_MODE_SINGLE_WAVE`: Single mode
    * `SENSEDU_DAC_MODE_BURST_WAVE`: Burst mode
* `burst_num`: Number of LUT cycles for `SENSEDU_DAC_MODE_BURST_WAVE` mode

#### Notes
{: .no_toc}
* `burst_num` is not ignored only for `SENSEDU_DAC_MODE_BURST_WAVE` mode. 


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
* Initializes associated DMA and timer.


### SensEdu_DAC_Enable
Enables DAC module, wave transmission starts.
```c
void SensEdu_DAC_Enable(DAC_Channel* dac_channel);
```
#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance

#### Notes
{: .no_toc}
* There is no separate `Enable` and `Start` function as for ADC.

### SensEdu_DAC_Disable
Deactivates DAC module. 

```c
void SensEdu_DAC_Disable(DAC_Channel* dac_channel);
```
#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance


### SensEdu_DAC_GetBurstCompleteFlag
Returns the burst status flag of the DAC channel. When transfer is finished, it returns `1`.

```c
uint8_t SensEdu_DAC_GetBurstCompleteFlag(DAC_Channel* dac_channel);
```

#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance

#### Returns
{: .no_toc}
* `burst_complete` flag: `1` indicates finished burst transfer


### SensEdu_DAC_ClearBurstCompleteFlag

Clears the burst status flag of the DAC channel to its default value `0`.

```c
void SensEdu_DAC_ClearBurstCompleteFlag(DAC_Channel* dac_channel);
```

#### Parameters
{: .no_toc}
* `dac_channel`: DAC Channel instance.

## Examples

Examples are organized incrementally. Each builds on the previous one by introducing only new features or modifications. Refer to earlier examples for core functionality details.
{: .fw-500}

If you want to see complete examples, visit `\examples\` directory or open them via Arduino IDE by navigating to `File → Examples → SensEdu`.

Each example uses a LUT with specified (12-bit) values and size. An example of defining a sine wave of 64 samples is shown in the following code snippet

```c
const SENSEDU_DAC_BUFFER(buffer_name, buffer_size) = {...};
```
where the first parameter of `SENSEDU_DAC_BUFFER` is the user-defined ***name*** to be used in the program code while the second parameter is the ***size*** of the LUT. 

{: .NOTE}
User can specify LUT size to be any positive integer. However, the real size of the DAC buffer has to be an integer divisible by integers raised by power of 2. See more details in [MPU Configuration]({% link Library/DAC.md %}#mpu-configuration) section.

### Send_DAC_Single_Sine

Transmitting a single instance of a predefined LUT with sine waveform.

1. Include SensEdu library
2. Declare DAC Buffer and initialize it with sine LUT
3. Initialize the `SensEdu_DAC_Settings` struct with DAC parameters.
4. Initialize `SensEdu_DAC_Init` with created struct
5. Enable wave transmission `SensEdu_DAC_Enable`

```c
#include <SensEdu.h>

const uint16_t sine_lut_size = 64; // sine wave size
const SENSEDU_DAC_BUFFER(sine_lut, sine_lut_size) = {
    0x0000,0x000a,0x0027,0x0058,0x009c,0x00f2,0x0159,0x01d1,
    0x0258,0x02ed,0x038e,0x043a,0x04f0,0x05ad,0x0670,0x0737,
    0x0800,0x08c8,0x098f,0x0a52,0x0b0f,0x0bc5,0x0c71,0x0d12,
    0x0da7,0x0e2e,0x0ea6,0x0f0d,0x0f63,0x0fa7,0x0fd8,0x0ff5,
    0x0fff,0x0ff5,0x0fd8,0x0fa7,0x0f63,0x0f0d,0x0ea6,0x0e2e,
    0x0da7,0x0d12,0x0c71,0x0bc5,0x0b0f,0x0a52,0x098f,0x08c8,
    0x0800,0x0737,0x0670,0x05ad,0x04f0,0x043a,0x038e,0x02ed,
    0x0258,0x01d1,0x0159,0x00f2,0x009c,0x0058,0x0027,0x000a
};

#define DAC_SINE_FREQ       32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle

DAC_Channel* dac_ch = DAC_CH1;
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = dac_ch, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)sine_lut,
    .mem_size = sine_lut_size,
    .wave_mode = SENSEDU_DAC_MODE_SINGLE_WAVE,
    .burst_num = 0
};

void setup() {
    SensEdu_DAC_Init(&dac_settings);
}

void loop() {
    SensEdu_DAC_Enable(dac_ch);
    delay(100);
}
```

#### Notes
{: .no_toc}
* In this example wave is sent every 100ms.
* If you put only `SensEdu_DAC_Enable` in setup, then the wave will be transmitted only once when you power up the board, so it is easily missable. If you want to see it with an oscilloscope, you could reset firmware by pressing `RST` button once on Arduino (do not press two times in succession, you will clear MCU firmware this way).


### Send_DAC_Burst_Sine

Transmitting a specified number of cycles of a predefined LUT with sine waveform, creating bursts.

1. Follow single wave example [`Send_DAC_Single_Sine`]({% link Library/DAC.md %}#send_dac_single_sine)
2. Change `wave_mode` to `SENSEDU_DAC_MODE_BURST_WAVE`
3. Specify `burst_num` to desired cycle number

```c
// DAC configuration struct
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = 10
```

### Send_DAC_Const_Sine

Transmitting a constant sine wave with predefined LUT.

1. Follow single wave example [`Send_DAC_Single_Sine`]({% link Library/DAC.md %}#send_dac_single_sine)
2. Change `wave_mode` to `SENSEDU_DAC_MODE_CONTINUOUS_WAVE`
3. Enable DAC once in setup with `SensEdu_DAC_Enable`

```c
// DAC configuration struct
    .wave_mode = SENSEDU_DAC_MODE_CONTINUOUS_WAVE,
    ...

void setup() {
    SensEdu_DAC_Init(&dac_settings);
    SensEdu_DAC_Enable(dac_ch);
}

void loop() {
    // nothing
}
```

### Send_DAC_Variable_Wave

Transmitting wave constantly with LUT changes during the program execution (run-time modifications). For this specific example we use small DAC buffer (4 elements) to generate a triangular wave across whole 12-bit region.

1. Include SensEdu library
2. Declare DAC Buffer and initialize it with any values
3. Initialize the `SensEdu_DAC_Settings` struct with DAC parameters for constant wave
4. Initialize `SensEdu_DAC_Init` with created struct and enable the wave transmission `SensEdu_DAC_Enable`
5. Modify LUT to create triangular shape by incrementing or decrementing each LUT element in a loop. When any value reaches 0 or 65535, change direction with `increment_flag`

```c
#include <SensEdu.h>

static uint8_t increment_flag = 1; // run time modification flag

const size_t lut_size = 4;
static SENSEDU_DAC_BUFFER(lut, lut_size) = {
    0x0000,0x0001,0x0002,0x0003
};

DAC_Channel* dac_ch = DAC_CH1;
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = dac_ch, 
    .sampling_freq = 64000*16, // ~1MHz sampling rate
    .mem_address = (uint16_t*)lut,
    .mem_size = lut_size,
    .wave_mode = SENSEDU_DAC_MODE_CONTINUOUS_WAVE,
    .burst_num = 0
};

void setup() {
    SensEdu_DAC_Init(&dac_settings);
    SensEdu_DAC_Enable(dac_ch);
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

    // increase\decrease change if out of bounds
    if (lut[0] == 0x0000) {
        increment_flag = 1;
    }
    if (lut[lut_size-1] == 0x0FFF) {
        increment_flag = 0;
    }
}
```

## Developer Notes

### DMA Streams

Each DAC channel occupies one DMA Stream:
* **Channel 1**: DMA1_Stream2
* **Channel 2**: DMA1_Stream3

{: .WARNING }
Avoid reusing occupied DMA streams. Refer to [STM32H747 Reference Manual] to find free available streams.

### Cache Coherence

When using DAC with DMA, you need to be aware of cache coherence problems. By default, the processor’s data cache (D-Cache) boosts memory access speed, but this can conflict with DMA operations. The DMA controller transfers data directly between memory and peripherals without CPU involvement. The issue arises when CPU interact with memory handled by DMA, the processor might read outdated data stored in cache instead of the actual data in memory, as it is not aware of DMA transfers. 

You can think that it shouldn't be a problem for DAC, since the data is written from memory to peripheral, CPU doesn't read anything. The problem arises, because default Arduino **MPU** (Memory Protection Unit) configuration enables write-back policy for writing operations. There are two possible policies:
* **Write-through policy (WT)**: Data is written to both cache and memory
* **Write-back policy (WB)**: Data is written to the cache first

That means if you use WB policy and update DAC buffer (waveform), DMA may not see updates **unless the cache is explicitly cleaned**.

There are two ways to fix this:
1. Cache Cleaning
2. MPU Configuration

{: .NOTE}
SensEdu uses MPU Configuration for the DAC.

#### Cache Cleaning
{: .no_toc}

After updating the DAC buffer, explicitly clean the cache to force writes to physical memory. Use the CMSIS function `SCB_CleanDCache_by_Addr(mem_addr, mem_size)` with the following parameters:
* `mem_addr`: Memory address of the DAC buffer
* `mem_size`: Memory size **in bytes**

```c
// Update Buffer
for (uint16_t i = 0; i < buf_size; i++) {
    buf[i] = i;
}
// Clean Cache and Start DAC
SCB_CleanDCache_by_Addr((uint16_t*)buf, sizeof(buf));
SensEdu_DAC_Enable(DAC_CH1);
```

The cache cleaning procedure applies to the entire cache line. Therefore, it is essential to align your DAC buffer to the cache line and ensure its size is a multiple of the cache line size. For the STM32H747, the cache line is 32 bytes long and is defined in the macro `__SCB_DCACHE_LINE_SIZE`. For a `uint16_t` array, this means the number of elements must be a multiple of 16 (each element is 2 bytes). For example. valid sizes include 16, 32, 64, etc. Alignment is achieved using the `__attribute__` directive:

```c
const uint16_t buf_size = 128; // multiple of __SCB_DCACHE_LINE_SIZE/2
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t buf[buf_size];
```

#### MPU Configuration
{: .no_toc}

To avoid manual cache maintenance, configure the MPU to mark the DAC buffer’s memory region as non-cacheable. This bypasses the cache entirely, ensuring DMA always accesses physical memory.

First, you need to ensure the correct buffer size. It must be a power of two (starting at 32 bytes) and aligned to its size. From STM32CubeMX MPU screenshot, you can see an example of proper sizes. 

![]({{site.baseurl}}/assets/images/MPU_size.png)

The SensEdu library automates this with the `SENSEDU_DAC_BUFFER(name, user_size)` macro:
* `name`: Variable name to access the buffer later in code
* `user_size`: buffer size **in uint16_t**

```c
const uint16_t buf_size = 50;
SENSEDU_DAC_BUFFER(buf, buf_size);
for (uint16_t i = 0; i < buf_size; i++) {
    buf[i] = i;
}
```

{: .NOTE}
The `SENSEDU_DAC_BUFFER` macro allows **any** user-defined size. The library internally adjusts it to meet MPU requirements.

After buffer allocation, during `DMA_DACInit()` the library configures the MPU region using internal function `LL_MPU_ConfigRegion()` to enforce non-cacheable and non-bufferable memory region.


[STM32H747 Reference Manual]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
[Reference Manual]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf