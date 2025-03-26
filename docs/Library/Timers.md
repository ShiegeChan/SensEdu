---
title: Timers
layout: default
parent: Library
math: mathjax
nav_order: 1
---

# Timers Peripheral
{: .fs-8 .fw-500 .no_toc}
---

Timers allow precise event scheduling, such as creating delays or setting sampling rates for peripherals.
{: .fw-500}

- TOC
{:toc}

## Errors

Main timers error code is `0x10xx`. Find the way to display errors in your Arduino sketch [here]({% link Library/index.md %}#error-handling).

An overview of possible errors for timers:
* `0x1000`: No Errors
* `0x1001`: Unexpected DAC frequency. Maximum possible is 60MHz, refer to [these calculations]({% link Library/Timers.md %}#frequency-settings) for more details

## Functions

### SensEdu_TIMER_DelayInit
Initializes the timer used for microsecond delays. Call it once in the setup before using delays.

```c
void SensEdu_TIMER_DelayInit();
```

### SensEdu_TIMER_Delay_us

Pauses program execution for a specified duration (blocking delay).

```c
void SensEdu_TIMER_Delay_us(uint32_t delay_us);
```

#### Parameters
{: .no_toc}
* `delay_us`: Delay duration in microseconds. Maximum: 4,294,967,295 µs.

## Examples

### Blink_Delay

Blinks a LED using microsecond delays.

1. Include the SensEdu library and declare the LED pin (`LED_BUILTIN` in this case)
2. Initialize the timer with `SensEdu_TIMER_DelayInit()`
3. Configure the LED pin as an `OUTPUT` using `pinMode()`
4. Use alternating calls to `SensEdu_TIMER_Delay_us()` and `digitalWrite()` in the main loop to toggle the LED state.

{: .NOTE}
On Arduino GIGA R1 the built-in LED is active-low: `LOW` = on, `HIGH` = off.

```c
#include "SensEdu.h"

uint8_t led = LED_BUILTIN;

void setup() {
    SensEdu_TIMER_DelayInit();

    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);
}

void loop() {
    SensEdu_TIMER_Delay_us(500000);
    digitalWrite(led, HIGH);
    SensEdu_TIMER_Delay_us(500000);
    digitalWrite(led, LOW);
}
```

## Developer Notes

### Timer occupation

Peripheral timers (ADC/DAC) are hidden, automatically configured and require no user involvement.

Timer allocation:
* **TIM1**: ADC sampling
* **TIM2**: delays 
* **TIM4**: DAC sampling 

{: .warning }
Avoid reusing occupied timers. Refer to [STM32H747 Reference Manual] to find free available timers. Be aware, future updates will assign dedicated timers to each ADC/DAC separately, which may occupy your custom timer.

### Frequency settings

Timer frequency is dependent on 3 parameters:
* Clock Frequency $$(CLK)$$
* Prescaler $$(PSC)$$: CLK divider
* Period $$(ARR)$$: Register containing the value up to which the count proceeds

$$TIM_{freq} = \frac{CLK}{PSC + 1} * \frac{1}{ARR + 1}$$

The CLK signal is derived from the APB1 and APB2 Timer Clocks, each running at 240MHz.

![]({{site.baseurl}}/assets/images/TIM_CLK.png)

Prescaler is set to its minimum value to achieve the finest frequency adjustments, resulting in a step size of $$\frac{1}{120\text{MHz}} \approx 8.33\text{ns}$$.

| ARR | Period | Frequency |
|:----|:-------|:----------|
| 1   | 16.7ns | 60MHz     |
| 2   | 25.0ns | 40MHz     |
| 3   | 33.3ns | 30MHz     |
| 4   | 41.7ns | 24MHz     |
| 5   | 50.0ns | 20MHz     |

{: .NOTE }
When a user specifies a frequency for a DAC or ADC, the target value is automatically rounded to the nearest achievable frequency dictated by the timer's step. The lower target frequency, the higher the achievable precision.

[STM32H747 Reference Manual]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
