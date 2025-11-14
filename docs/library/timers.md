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

The main timer error code prefix is `0x10xx`. Find the way to display errors in your Arduino sketch [here]({% link Library/index.md %}#error-handling).

An overview of possible errors for timers:
* `0x1000`: No Errors
* `0x1001`: Unexpected delay value. Minimum possible is 1ns
* `0x1002`: Unexpected ADC frequency. Maximum possible is 120MHz, refer to [these calculations]({% link Library/Timers.md %}#frequency-settings) for more details
* `0x1003`: Unexpected DAC frequency. Maximum possible is 60MHz, refer to [these calculations]({% link Library/Timers.md %}#frequency-settings) for more details
* `0x1004`: TIM8 initialization attempt while TIM8 is running. Configuration is possible only for disabled timer
* `0x1005`: TIM8 Unexpected CCR channel. Possible options are: `CCR1`, `CCR2`, `CCR3` or `CCR4`

An overview of critical errors. They shouldn't happen in normal user case and indicate some problems in library code:
* `0x10A0`: Timer frequency calculations failed

## Functions

### SensEdu_TIMER_DelayInit
Initializes the timer used for microsecond delays. Call it once in the setup before using delays.

```c
void SensEdu_TIMER_DelayInit();
```

### SensEdu_TIMER_Delay_us

Pauses program execution for a specified duration in microseconds (blocking delay).

```c
void SensEdu_TIMER_Delay_us(uint32_t delay_us);
```

#### Parameters
{: .no_toc}
* `delay_us`: Delay duration in microseconds. Maximum: $$4,294,967,295 \text{us}$$.

#### Notes
{: .no_toc}
* For `delay_us` $$\leq$$ $$1000\text{us}$$, the function internally switches to `SensEdu_TIMER_Delay_ns()`, to offer more precise timings with software overhead compensation.

### SensEdu_TIMER_Delay_ns

Pauses program execution for a specified duration in nanoseconds (blocking delay).

{: .WARNING}
This is an experimental function. Read the notes carefully.

```c
void SensEdu_TIMER_Delay_ns(uint32_t delay_ns);
```

#### Parameters
{: .no_toc}
* `delay_ns`: Delay duration in nanoseconds. Maximum: $$4,294,967,295 \text{ns}$$.

#### Notes
{: .no_toc}
* For `delay_ns` $$\gt$$ $$1000\text{us}$$, the function internally switches to `SensEdu_TIMER_Delay_us()` to avoid potential 32-bit overflow in the `NS_TO_TICKS(ns)` macro.
* The lowest achievable timer resolution on STM32H747 MCU is ~$$4.17\text{ns}$$, calculated as: $$\text{tick} = 1/240\text{MHz} \times 10^9 \approx4.17\text{ns}$$. Delays are therefore multiples of this tick, approximately: $$4\text{ns}$$, $$8\text{ns}$$, $$13\text{ns}$$, $$17\text{ns}$$, etc.
* Delays shorter than $$250\text{ns}$$ are generally not practical, due to set frequencies being close to the system clock frequency of the MCU. These delays become overpowered by the software overhead. Execution of the function itself takes around $$60-120$$ CPU cycles, corresponding to ~$$125-250\text{ns}$$ on a $$480\text{MHz}$$ core.
* To account for software overhead, a hardcoded compensation of $$550\text{ns}$$ is applied. Additionally, any requested delays below this threshold are automatically raised to $$550\text{ns}$$. The reasons for this exact number are explained in the [compensation section]({% link Library/Timers.md %}#software-overhead-compensation-in-nanosecond-delays).

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
* **TIM2**: Delays 
* **TIM4**: DAC sampling
* **TIM8**: PWM

{: .WARNING }
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

### Software Overhead Compensation in Nanosecond Delays

Software execution adds an unavoidable overhead, which becomes noticeable for very small delays in the range of hundreds of nanoseconds. To measure this, a simple test was performed:

```c
SensEdu_TIMER_Delay_ns(1);
digitalWrite(pin, HIGH);
SensEdu_TIMER_Delay_ns(1);
digitalWrite(pin, LOW);
```

{: .WARNING}
This test is not fully precise, since `digitalWrite()` itself introduces an additional delay. However, because digital I/O is a typical use case for these delays, the results are considered representative.

The following figure shows the resulting waveform for a set delay of $$1\text{ns}$$, followed by a summary table with measurement results for various delays.

![]({{site.baseurl}}/assets/images/timer_soft_overhead_1ns.png)

| Set Delay | Actual Delay | Software Overhead |
|:----------|:-------------|:------------------|
| 1ns       | 546ns        | 545ns             |
| 250ns     | 788ns        | 538ns             |
| 1000ns    | 1537ns       | 537ns             |
| 5000ns    | 5522ns       | 522ns             |

Based on these results, the rounded average execution overhead is estimated as $$550\text{ns}$$. This value is defined in `timer.c` as the macro `DELAY_CPU_OVERHEAD_NS` and is automatically subtracted from the requested nanosecond delay. The figure below shows a requested delay of $$5000\text{ns}$$: **without compensation (black)**{: .text-black-000} and **with compensation (blue)**{: .text-blue-000}. While compensation is applied, the delay averages ~$$5004\text{ns}$$.

![]({{site.baseurl}}/assets/images/timer_soft_overhead_compensated.png)


[STM32H747 Reference Manual]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
