---
title: Library
layout: default
parent: Contributing
nav_order: 2
---

# Library Contributions
{: .fs-8 .fw-500 .no_toc}
---

We highly welcome any code improvements from more experienced embedded developers. The library, designed as a custom hardware abstraction layer for the STM32H747, is created with simplicity and flexibility in mind. It directly interfaces with hardware registers, avoiding the use of the STM32 HAL.
{: .fw-500}

- TOC
{:toc}

## Style Guidelines

The library is written in C. Source files must end in `.c`, and header files in `.h`. Every peripheral is one `{peripheral}.c` / `{peripheral}.h` pair inside `libraries/SensEdu/src/`, tied together by `SensEdu.h`:

```
libraries/SensEdu/
├── library.properties      Arduino library metadata, bump the version on release
├── keywords.txt            IDE syntax highlighting for the public API
├── examples/               one folder per example sketch
└── src/
    ├── SensEdu.h           user-facing header, error ranges
    ├── libs.h              shared includes
    ├── adc.c   adc.h
    ├── dac.c   dac.h
    ├── dma.c   dma.h
    ├── pwm.c   pwm.h
    └── timer.c timer.h
```

{: .NOTE}
This page covers the C sources in `src/` only. The example sketches in `examples/` are regular Arduino sketches, they follow the [project style guidelines]({% link contributing/projects.md %}#style-guidelines) like everything under `projects/`.

### Indentation

Like the rest of the repository, the library uses 4-space indentation.

### Header Files

Each header opens with a Doxygen-style file comment.

```c
/**
 * @file dac.h
 * @brief Public API for DAC driver (DMA-based waveform generation).
 *
 * This module provides:
 * - DAC waveform generation using DMA
 * - Timer-triggered DAC sampling
 * - Burst and continuous waveform modes
 *
 * User constraints:
 * - SensEdu_DAC_Init() must be called before Enable()
 * - Both channels must use identical sampling_freq
 *
 * Notes:
 * - Both DAC channels share the same timer, meaning they MUST use the same sampling frequency
 * - Calling Enable() starts DMA transfers, not the timer
 * - After a burst completes, the DAC must be re-enabled to restart output
 */
```

Each `.h` file should use an include guard (`#ifndef`/`#define`/`#endif`) to prevent accidental double-inclusion. Additionally, wrap headers in an `extern "C"` block, due to Arduino being a C++ environment. A typical header file should look like this:

```c
#ifndef __NEW_HEADER_H__
#define __NEW_HEADER_H__

#include "libs.h"

#ifdef __cplusplus
extern "C" {
#endif

// file contents: defines, enums, structs, declarations

#ifdef __cplusplus
}
#endif

#endif // __NEW_HEADER_H__
```

Inside the guard, keep a fixed order, so a reader always finds things in the same place:

1. `#define`s
2. The error enum, see [Error Handling](#error-handling)
3. Mode enums
4. Settings structs
5. User API declarations (`SensEdu_` prefixed), grouped by purpose
6. Library-internal declarations (`{Peripheral}_` prefixed)

```c
typedef enum {
    SENSEDU_ADC_SR_MODE_FREE = 0x00,    // As fast as possible
    SENSEDU_ADC_SR_MODE_FIXED = 0x01    // Fixed timer-triggered rate
} SENSEDU_ADC_SR_MODE;

typedef struct {
    ADC_TypeDef* adc;               // ADC instance (ADC1/2/3)
    uint8_t* pins;                  // Array of pins to sample
    uint8_t pin_num;                // Number of pins in pin array

    SENSEDU_ADC_SR_MODE sr_mode;    // FREE: free-run; FIXED: with timer-triggered rate
    uint32_t sampling_rate_hz;      // SR in Hz (if sr_mode = FIXED)
    ...
} SensEdu_ADC_Settings;
```

### Source Files

A `.c` file repeats the file comment of its header with `@brief Internal implementation of ...`, followed by the includes. Everything after that is split into sections by a banner:

```c
/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
```

Typically used banners:

| Section | Contents |
|:--------|:---------|
| `Constants` | `#define`s and `static const` values |
| `Structs` | Driver-private `typedef struct`s |
| `Maps` | `static const` lookup tables (pin-to-channel, register offsets, ...) |
| `Variables` | `static` variables, starting with the error container |
| `Declarations` | Forward declarations of every private function |
| `Public Functions` | `SensEdu_` and `{Peripheral}_` definitions, in header order |
| `Private Functions` | `static` definitions, in declaration order |
| `Interrupts` | Interrupt handlers |

```c
/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

// Global error container
static ADC_ERROR error = ADC_ERROR_NO_ERRORS;

// Per ADC storage containers
static SensEdu_ADC_Settings adc_settings[3];
static AdcState adc_states[3];

// Clock config flag
static bool pll_configured = false;
```

### Naming Convention

* **Macros**: `SCREAMING_SNAKE_CASE`
* **Constants**: `SCREAMING_SNAKE_CASE`
* **Variables**: `snake_case`
  
```c
#define SAMPLE_FREQUENCY 1000
const uint16_t SOUND_SPEED = 343;
uint16_t buf;
```
* **Enums**: `SCREAMING_SNAKE_CASE`
* **Structs**: `PascalCase`

```c
typedef enum {
    ADC_ERROR_NO_ERRORS = 0x00,
    ADC_ERROR_TOO_HIGH_FREQUENCY = 0x01,
    ADC_ERROR_CONVERSION_FAILED = 0x02
} ADC_ERROR;

typedef struct {
    uint8_t num;
    uint32_t presel;
} ChannelSelector;
```

* **Private Functions**: `snake_case` (local to the source file)
* **Public Functions**: `PascalCase` (exposed via header inclusion)

```c
static void configure_clock(void);
void WriteValue(uint16_t value);
```

### Namespace

Ensure that all publicly available functions and structs for a specific peripheral start with its namespace using the format `{Peripheral}_{FunctionName}`.

```c
typedef struct {
    ...
} ADC_Channel;

ADC_ERROR ADC_GetError(void);
```

If you want to make any function or struct additionally accessible for Arduino users, add the `SensEdu_` prefix.

```c
typedef struct {
    ...
} SensEdu_ADC_Settings;

void SensEdu_ADC_Enable(ADC_TypeDef* ADC);
void SensEdu_ADC_Start(ADC_TypeDef* ADC);
```

* **Private functions:** Accessible exclusively within the source file \
`static void configure_clock(void);`
* **Public library functions:** Accessible throughout the library \
`ADC_ERROR ADC_GetError(void);`
* **Public user functions:** Accessible to users in Arduino sketches \
`void SensEdu_ADC_Enable(ADC_TypeDef* ADC);`

### Error Handling

Every driver declares an error enum in its header. Values `0x00`-`0x9F` describe user mistakes, such as bad settings, a wrong peripheral instance or an illegal mode combination. Values from `0xA0` upwards are reserved for critical errors, they should never be reachable from user code and always indicate a bug in the library:

```c
typedef enum {
    DAC_ERROR_NO_ERRORS = 0x00,
    DAC_ERROR_NULL_INPUT_SETTINGS = 0x01,
    DAC_ERROR_ALREADY_ENABLED = 0x02,
    DAC_ERROR_WRONG_DAC_CHANNEL = 0x03,

    DAC_ERROR_DMA_UNDERRUN = 0xA0,
    DAC_ERROR_TIMEOUT = 0xA1
} DAC_ERROR;
```

The driver keeps a single `static` error container in the `Variables` section and exposes it through `{Peripheral}_GetError()`. Public functions validate their inputs first, record the error, and return without touching the hardware:

```c
void SensEdu_ADC_Init(SensEdu_ADC_Settings* new_settings) {

    // Sanity checks
    if (new_settings == NULL) {
        error = ADC_ERROR_NULL_INPUT_SETTINGS;
        return;
    }
    error = check_settings(new_settings);
    if (error != ADC_ERROR_NO_ERRORS) return;
    ...
}
```

Each peripheral owns a `0xN000` range in the `SENSEDU_ERROR` enum in `src/SensEdu.h`, which is what `SensEdu_GetError()` reports to the sketch. Register a range for every new peripheral and document every code in the [library wiki]({% link library/index.md %}#error-handling).

### Reference Headers

The library builds on the CMSIS device headers that ship inside the Arduino core, not on a separately installed STM32CubeH7 package. `src/libs.h` already includes them, so everything listed below is available in any library source file.

They are located at:
- Windows: `C:\Users\{username}\AppData\Local\Arduino15\packages\arduino\hardware\mbed_giga\{version}\`
- Linux: `/home/{username}/.arduino15/packages/arduino/hardware/mbed_giga/{version}/`
- macOS: `/Users/{username}/Library/Arduino15/packages/arduino/hardware/mbed_giga/{version}/`

Where `{version}` is the installed version of the *Arduino Mbed OS GIGA Boards* package:

| Contents | Path |
|:-------------------------|:----------------------------|
| Register masks and positions (`ADC_CR_ADEN`, `ADC_ISR_OVR_Msk`, `ADC_SQR1_SQ1_Pos`), peripheral structs (`ADC_TypeDef`), instance pointers (`ADC1`, `GPIOJ`) and the `IRQn_Type` enum | `./cores/arduino/mbed/targets/TARGET_STM/TARGET_STM32H7/STM32Cube_FW/CMSIS/stm32h747xx.h` |
| The register macros `SET_BIT`, `CLEAR_BIT`, `READ_BIT`, `WRITE_REG`, `READ_REG`, `MODIFY_REG` | `./cores/.../STM32Cube_FW/CMSIS/stm32h7xx.h` |
| LL helpers included by `libs.h` (`stm32h7xx_ll_tim.h`) | `./cores/.../STM32Cube_FW/STM32H7xx_HAL_Driver/` |
| Core intrinsics `NVIC_EnableIRQ`, `NVIC_SetPriority`, `__NOP`, `__disable_irq` | `./cores/arduino/mbed/cmsis/CMSIS_5/CMSIS/TARGET_CORTEX_M/Include/core_cm7.h` |

{: .NOTE}
The core flattens ST's folder structure. The LL headers sit directly inside `STM32H7xx_HAL_Driver/`, there is no `Inc/` subfolder like in a stock STM32CubeH7 package. ST ships the LL and HAL drivers in one package, which is why the folder is named after the HAL even though the library does not use it.

Every bitfield in `stm32h747xx.h` comes as a triplet, so the plain name is the ready-to-use mask:

```c
#define ADC_CR_ADEN_Pos     (0U)
#define ADC_CR_ADEN_Msk     (0x1UL << ADC_CR_ADEN_Pos)
#define ADC_CR_ADEN         ADC_CR_ADEN_Msk
```

Use the plain mask with `SET_BIT` and `READ_BIT`, and the `_Msk` / `_Pos` pair with `MODIFY_REG` when you need to place a value into a multi-bit field.

### Register Access

Registers are manipulated with the CMSIS macros `SET_BIT`, `CLEAR_BIT`, `READ_BIT`, `READ_REG`, `WRITE_REG` and `MODIFY_REG`, using STM32H747 low level library `_Msk` and `_Pos` definitions, see [Reference Headers](#reference-headers):

```c
SET_BIT(adc->CR, ADC_CR_ADEN);
MODIFY_REG(gpio->MODER, 0x3UL << shift, 0b10 << shift);
```

Never wait for a hardware flag forever. Bound every wait with a counter and report `{Peripheral}_ERROR_TIMEOUT`:

```c
uint32_t timeout = UINT32_MAX;
while (!READ_BIT(adc->ISR, ADC_ISR_ADRDY) && timeout--) {
    __NOP();
}
if (timeout == 0) {
    error = ADC_ERROR_TIMEOUT;
    return;
}
```

{: .IMPORTANT}
A blocking loop without a timeout will hang the user's sketch with no diagnostics at all.

### Interrupts

Interrupt handlers live at the very end of the file under the `Interrupts` banner and use the CMSIS vector name (`ADC_IRQHandler`, `DMA1_Stream5_IRQHandler`, `TIM2_IRQHandler`), see [Interrupt Vector Names](#interrupt-vector-names). Keep them short: update a flag or counter, clear the hardware flag, return. Actual processing belongs in the main loop of the sketch.

State shared between an interrupt and the main loop must be declared `volatile` to avoid compiler optimization for this variable.

```c
// Describes per ADC runtime state
typedef struct {
    volatile bool ovr_flag;                 // Flag notifying that overrun event happened
    volatile uint32_t ovr_counter;          // OVR event counter
    volatile bool dma_complete;             // DMA transfer complete flag
    volatile bool dma_half_transfer;        // DMA half transfer reached flag
    uint16_t seq_buffer[MAX_CHANNEL_NUM];   // software polling data storage
} AdcState;
```

### Interrupt Vector Names

The GIGA core ships Mbed OS as a precompiled library, so there is no `startup_stm32h747xx.s` in the core to read the vector table from. Derive the handler name from the `IRQn_Type` enum in `stm32h747xx.h` instead, by replacing the `_IRQn` suffix with `_IRQHandler`:

```c
DMA1_Stream5_IRQn = 16,   /*!< DMA1 Stream 5 global Interrupt   */   ->   DMA1_Stream5_IRQHandler()
ADC_IRQn          = 18,   /*!< ADC1 and ADC2 global Interrupts  */   ->   ADC_IRQHandler()
ADC3_IRQn         = 127,  /*!< ADC3 global Interrupt            */   ->   ADC3_IRQHandler()
```

The comments in the enum also tell you which peripherals share a vector. `ADC_IRQn` covers both ADC1 and ADC2, while ADC3 has its own entry, which is why `adc.c` implements two separate handlers.

{: .WARNING}
A misspelled handler name still compiles, so your code never runs and the interrupt flag is never cleared.

### Braces and Spaces

* Place the opening brace on the same line as the function name
* Put one space before the opening brace
* Put one space after the keywords `if`, `switch`, `case`, `for`, `do`, `while`
* Do not put a space after the function in both calls and declarations
* Preferred use of pointer's `*` is adjacent to the data type

```c
function(char* ptr) {
    if (this_is_true) {
        do_something(ptr);
    } else {
        do_something_else(ptr);
    }
}
```

* Put spaces around most operators
* It is okay to omit spaces around factors for readability

```c
// Good examples
v = w * x + y / z;
v = w*x + y/z;
v = w * (x + z);

// Bad examples
v = x+y;           // use spaces around operators
v = w*x + y / z;   // don't mix styles
v = w * ( x + z ); // no internal padding for parentheses
```

### Supplementary Resources
* If a topic isn't covered here, refer to the [Linux Kernel Coding Style](https://www.kernel.org/doc/html/v4.10/process/coding-style.html). Local rules in this document take precedence (e.g., 4-space indentation).


## Adding a New Peripheral

1. Create `{peripheral}.c` and `{peripheral}.h` with the file comment, the error enum and the section banners described above
2. Include the header in `src/SensEdu.h` and add a `SENSEDU_ERROR_{PERIPHERAL} = 0xN000` entry to the `SENSEDU_ERROR` enum
3. Add the public function names to `libraries/SensEdu/keywords.txt` so the Arduino IDE highlights them
4. Add at least one example under `libraries/SensEdu/examples/` that demonstrates the new functionality, following the [example style]({% link contributing/projects.md %}#style-guidelines)
5. Create `docs/library/{peripheral}.md` and link it from `docs/library/index.md` under "Supported Peripherals"

### Writing the Wiki Page

Peripheral pages all follow the same layout, so that users can jump between them without re-learning the structure. Use an existing page such as [ADC]({% link library/adc.md %}) as the template:

1. **Errors**: the `0xN0xx` range, split into user errors and critical errors, one bullet per code
2. **Structs**: the settings struct, a `#### Fields` list, and a `#### Notes` block for hardware constraints (pin maps, shared resources)
3. **Functions**: one `###` heading per public function with a one-line description, the C signature in a code block, then `#### Parameters`, `#### Returns` and `#### Notes`
4. **Examples**: every example sketch that uses the peripheral, with the source and a short explanation
5. **Developer Notes**: register-level details, timing formulas and the reasoning behind the implementation

{: .TIP}
Add `{: .no_toc}` under the `####` headings so that the table of contents stays readable.

### Additional Information

Be cautious when using new timers or DMA streams, ensure they are actually available. Read through developer notes for each peripheral you intend to modify.

If you create any new major functionality, make sure to provide an example that demonstrates its usage.
{: .fw-500}

{: .WARNING}
Before pushing your changes to the library, verify that **all** examples work as intended to avoid unexpected errors.