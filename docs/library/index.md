---
title: Library
layout: default
nav_order: 4
permalink: /library/
---

# SensEdu Hardware Abstraction Library
{: .fs-8 .fw-500}
---

The SensEdu library provides a user-friendly interface for project development on the STM32H747 Arduino GIGA R1, while retaining low-level control for advanced customization. Designed for flexibility, avoiding STM32 HAL. It directly interfaces with hardware registers using [STM32H747 Reference Manual] and [ARMv7-M Architecture Reference Manual].
{: .fw-500}

## Overview

### Naming Convention

All public functions accessible in Arduino sketches follow the naming pattern:
`SensEdu_{Peripheral}_{Action}`

* **Peripheral**: Hardware module (e.g., `TIMER`, `ADC`, `DAC`)
* **Action**: Operation (e.g., `Init`, `Enable`, `Start`)

Example:
```c
SensEdu_ADC_Enable(ADC1); // Enable ADC1 peripheral
```

### Supported Peripherals
* [Timers]({% link Library/Timers.md %})
  * Prefix: `SensEdu_TIMER_`
  * Source: `\src\timer.c`
* [ADC]({% link Library/ADC.md %})
  * Prefix: `SensEdu_ADC_`
  * Source: `\src\adc.c` and `\src\dma.c`
* [DAC]({% link Library/DAC.md %})
  * Prefix: `SensEdu_DAC_`
  * Source: `\src\dac.c` and `\src\dma.c`
* [PWM]()
  * Prefix: `SensEdu_PWM_`
  * Source: `\src\pwm.c`

You can explore all available functions in the respective wiki sections or by looking into the header files (e.g., `\src\adc.h`):

```c
void SensEdu_ADC_Init(SensEdu_ADC_Settings* adc_settings);
void SensEdu_ADC_Enable(ADC_TypeDef* ADC);
void SensEdu_ADC_Disable(ADC_TypeDef* ADC);
void SensEdu_ADC_...
```

{: .IMPORTANT }
Lowercase functions (e.g., `configure_pll2()`) are private for source files. Non-prefixed with `SensEdu_` uppercase functions (e.g., `ADC_GetError()`) are used publically across the library, but not intended for user code. Avoid using them directly.

## Error Handling

Each peripheral defines a set of error codes to handle conditions like invalid input values or incorrect addresses. In the peripheral header files (e.g., `\src\adc.h`), there is the enumeration that lists the specific error codes. The error code for peripheral itself is defined in `\src\SensEdu.h`.

```c
// \src\adc.h
typedef enum {
    ADC_ERROR_NO_ERRORS = 0x00,
    ADC_ERROR_PLL_CONFIG = 0x01,
    ADC_ERROR_ADC_CONFIG_VOLTAGE_REGULATOR = 0x02,
    ADC_ERROR_ADC_DISABLE_FAIL = 0x03,
    ...
} ADC_ERROR;

// \src\SensEdu.h
typedef enum {
    SENSEDU_NO_ERRORS = 0x0000,
    SENSEDU_ERROR_TIMER = 0x1000,
    SENSEDU_ERROR_ADC = 0x2000,
    SENSEDU_ERROR_DMA = 0x3000,
    SENSEDU_ERROR_DAC = 0x4000,
    SENSEDU_ERROR_PWM = 0x5000
} SENSEDU_ERROR;
```

This way errors are encoded as 16-bit values:
* **Upper byte**: Peripheral identifier (e.g., 0x2000 for ADC)
* **Lower byte**: Specific error code within the peripheral

Example:
{:.mb-1}

* `0x2001`: ADC error `0x01` - Failed PLL configuration
* `0x4004`: DAC error `0x04` - DAC was already enabled before initialization

In the Arduino environment, call `SENSEDU_ERROR SensEdu_GetError();` to retrieve the latest error.

{: .TIP }
It is recommended to check for errors after critical operations (e.g., during initialization) and periodically in the main loop if real-time performance allows.

Below is an example of how to check for errors. Serial Monitor is being used to notify the programmer of any internal errors. Alternatively, you could use a LED (e.g., the on-board red LED on pin D86).

```c
void setup {
    ...
    Serial.begin(115200);

    SENSEDU_ERROR lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        // infinite loop if any errors
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
        delay(1000);
    }
}
```

## Examples

The best way to understand how to use the library is to review the provided examples. Each example targets a specific peripheral and demonstrates its key features.

You can find these examples in the `\examples\` directory or access them directly via the Arduino IDE by navigating to `File → Examples → SensEdu`. Detailed explanations for each example are available in the respective sections of this documentation.

If you still have any issues or have further questions, please reach out to us in [GitHub Discussions]!

## Useful Resources

* [A bare metal programming guide] by [Sergey Lyubka]
* [“Bare Metal” STM32 Programming] by [Vivonomicon]
* [STM32H7 ADC + DMA + Timer Firmware Tutorial] by [Phil’s Lab]


[STM32H747 Reference Manual]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
[STM32H747 Datasheet]: https://www.st.com/resource/en/datasheet/stm32h747ag.pdf
[ARMv7-M Architecture Reference Manual]: https://developer.arm.com/documentation/ddi0403/latest/
[GitHub Discussions]: https://github.com/ShiegeChan/SensEdu/discussions

[A bare metal programming guide]: https://github.com/cpq/bare-metal-programming-guide
[Sergey Lyubka]: https://github.com/cpq
[“Bare Metal” STM32 Programming]: https://vivonomicon.com/2018/04/02/bare-metal-stm32-programming-part-1-hello-arm/
[Vivonomicon]: https://vivonomicon.com/
[STM32H7 ADC + DMA + Timer Firmware Tutorial]: https://youtu.be/_K3GvQkyarg?si=HganXVK1rRaDIXO4
[Phil’s Lab]: https://www.youtube.com/@PhilsLab