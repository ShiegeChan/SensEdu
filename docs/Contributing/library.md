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

### General Guidelines

The library is written in C. Source file names must end in `.c`, and header files in `.h`.

Each `.h` file should include `#define` lines to prevent accidental double-inclusion. Additionally, header files should include an `extern "C"` wrapper, due to Arduino being a C++ environment. A typical header file should look like this:

```c
#ifndef __NEW_HEADER_H__
#define __NEW_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

// file contents: includes, declarations

#ifdef __cplusplus
}
#endif

#endif // __NEW_HEADER_H__
```

### Naming Convention

Certain names are divided into *Private* and *Public*, defined as:
* **Private:** Uses the `static` keyword, not declared in the header file, making it local to the source file
* **Public:** Available for use in other files

Naming rules:

* **Macros:** `SCREAMING_SNAKE_CASE`
* **Constant variables:** `SCREAMING_SNAKE_CASE`
* **Variables:** `snake_case`
  
```c
#define SAMPLE_FREQUENCY 1000
const uint16_t SOUND_SPEED = 343;
uint16_t buf;
```
* **Enums:** `SCREAMING_SNAKE_CASE`
* **Structs:** `PascalCase`

```c
typedef enum {
    ADC_ERROR_NO_ERRORS = 0x00,
    ADC_ERROR_TOO_HIGH_FREQUENCY = 0x01,
    ADC_ERROR_CONVERSION_FAILED = 0x02
} ADC_ERROR;

typedef struct {
    uint8_t num;
    uint32_t presel;
} ChannelSelector
```
* **Functions:** 
  * *Private:* `snake_case`
  * *Public:* `PascalCase`

```c
static void configure_clock(void);
void WriteValue(uint16_t value);
```

### Namespace

Ensure that all publicly available functions and structs for a specific peripheral start with its namespace: `{Peripheral}_{FunctionName}`.
{: .fw-400}

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

**In summary:**
* **Private functions:** Accessible exclusively within one source file - `static void configure_clock(void);`
* **Public library functions:** Accessible throughout the library - `ADC_ERROR ADC_GetError(void);`
* **Public user functions:** Accessible to users in Arduino sketches - `void SensEdu_ADC_Enable(ADC_TypeDef* ADC);`


### Additional Information

Be cautious when using new timers and DMA streams, ensure they are available. Read through developer notes for each peripheral you intend to modify.

If you create any new major functionality, make sure to provide an example that demonstrates its usage.
{: .fw-500}

{: .warning}
Before pushing your changes to the library, verify that **all** examples work as intended to avoid unexpected errors.