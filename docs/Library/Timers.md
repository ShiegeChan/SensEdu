---
title: Timers
layout: default
parent: Library
nav_order: 1
---

# Timers Peripheral
{: .fs-8 .fw-500 .no_toc}
---

Timers allow precise event scheduling, such as creating delays or setting sampling rates for peripherals.
{: .fw-500}

- TOC
{:toc}

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

{: .warning }
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

Peripheral timers (ADC/DAC) are hidden, automatically configured and require no user involvement.

Timer allocation:
* **TIM1**: ADC sampling
* **TIM2**: delays 
* **TIM4**: DAC sampling 

{: .warning }
Avoid reusing occupied timers. Refere to [STM32H747 Reference Manual] to find free available timers. Be aware, future updates will assign dedicated timers to each ADC/DAC separately, which may occupy your custom timer.

TODO: explain how frequency is set and what are the expected frequencies

[STM32H747 Reference Manual]: https://www.st.com/resource/en/reference_manual/
