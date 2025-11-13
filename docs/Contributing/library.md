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

### Style Guidelines

The library is written in C. Source files must end in `.c`, and header files in `.h`.

Each `.h` file should use an include guard (`#ifndef`/`#define`/`#endif`) to prevent accidental double-inclusion. Additionally, wrap headers in an `extern "C"` block, due to Arduino being a C++ environment. A typical header file should look like this:

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

#### Naming Convention

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
* **Functions:** 
  * *Private:* `snake_case` (local to the source file)
  * *Public:* `PascalCase` (exposed via header inclusion)

```c
static void configure_clock(void);
void WriteValue(uint16_t value);
```

#### Namespace

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

#### Braces and Spaces

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

#### Supplementary Resources
* If a topic isn't covered here, refer to the [Linux Kernel Coding Style](https://www.kernel.org/doc/html/v4.10/process/coding-style.html). Local rules in this document take precedence (e.g., 4-space indentation).


### Additional Information

Be cautious when using new timers and DMA streams, ensure they are available. Read through developer notes for each peripheral you intend to modify.

If you create any new major functionality, make sure to provide an example that demonstrates its usage.
{: .fw-500}

{: .WARNING}
Before pushing your changes to the library, verify that **all** examples work as intended to avoid unexpected errors.