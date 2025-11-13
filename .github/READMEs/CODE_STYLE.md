## SensEdu Projects

### General Guidelines

Arduino projects are written in C++. A source file is called a "sketch", it ends with `.ino` and must be in a folder with the same name. The project folder can contain additional `.ino` files to split functionality and make the code easier to manage. You can also include traditional `.h` header files for definitions and declarations.

Unlike standard Arduino code, SensEdu uses 4-space indentation. Please include the file `.theia/settings.json` with the following contents:
```json
{
  "editor.tabSize": 4
}
```

### Naming Convention

* **Macros**: `SCREAMING_SNAKE_CASE`
* **Constants**: `SCREAMING_SNAKE_CASE`
* **Variables**: `snake_case`
* **Functions**: `snake_case`
  
```c
#define AIR_SPEED 343
const uint16_t ADC_MIC_NUM = 4;
uint16_t buf;
void process_data(uint16_t* buf);
```

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
* If a topic isn’t covered here, refer to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). Local rules in this document take precedence (e.g., 4-space indentation).

## SensEdu Library

### General Guidelines

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
* **Functions:** 
  * *Private:* `snake_case` (local to the source file)
  * *Public:* `PascalCase` (exposed via header inclusion)

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