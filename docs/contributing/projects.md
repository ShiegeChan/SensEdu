---
title: Projects
layout: default
parent: Contributing
nav_order: 2
---

# Project Contributions
{: .fs-8 .fw-500 .no_toc}
---

We are excited to see the innovative ways in which you are utilizing SensEdu. By submitting your project, you have the opportunity to have it listed as a showcase for SensEdu functionality and included in the main repository.
{: .fw-500}

- TOC
{:toc}

## Project Submission

1. Fork [SensEdu repository]
2. Add a new project folder in your fork `~\projects\my_project\`
3. Create an Arduino sketch `~\projects\my_project\my_project.ino`. Ensure that the sketch filename matches the folder name
   1. If you have additional files (e.g., MATLAB scripts), place them in the subfolder within your project folder, such as `~\projects\my_project\matlab\my_script.m`
4. Create a documentation for your project
   1. Create a new markdown page `~\docs\projects\my_project.md`
   2. Add the following fields to the markdown header:
   ```md
   ---
   title: My Project
   layout: default
   parent: Projects
   nav_order: 10
   ---
   ```
   3. The order of projects is defined by `nav_order`. Use the next available number for proper website navigation. Customize `title` with your project name
   4. Follow [Documentation Contributions]({% link contributing/docs.md %}) for detailed page creation guidelines
   5. Share implementation details and nice pictures of your project!
5. Commit all changes to your fork and submit a [Pull Request] (PR) to the main SensEdu [repository]

A finished project folder looks like this:

```
projects/My_Project/
├── .theia/settings.json     4-space indentation for the Arduino editors
├── My_Project.ino           entry point, same name as the folder
├── helpers.ino              optional, additional sketch files
├── table.h                  optional, tables or declarations
└── matlab/                  matlab host scripts (can be python/)
    ├── .gitignore           ignores generated data (Recordings/, Measurements/, ...)
    └── My_Project.m
```

{: .IMPORTANT}
Never commit generated measurement data, recordings, figures or MATLAB autosave files. Add a small `.gitignore` next to the host script.

### Writing the Project Page

Project pages follow a common layout so that a reader can compare projects without re-learning the structure. Use an existing page such as [Audio Recording]({% link projects/audio-recording.md %}) as the template:

1. **Introduction**: what the project does and which problem it solves
2. **Background**: the theory
3. **Code Layout**: a table of every file with a one-line purpose
4. **Configuration**: a table of every tunable constant with its default and meaning, both firmware and host side
5. **Implementation**: some tricky parts of the firmware and the host script, with short code snippets rather than full source code
6. **Showcase**: plots, photos, audio, or measurements from a real run
7. **Developer Notes**: the non-obvious decisions and the reasoning behind them

{: .IMPORTANT}
The documentation is aimed at beginner students as well as developers. Explain the reasoning behind your design choices and avoid logical skips.

## Style Guidelines

Arduino projects are written in C++. A source file is called a "sketch", it ends with `.ino` and must be in a folder with the same name. The project folder can contain additional `.ino` files to split functionality and make the code easier to manage. You can also include traditional `.h` header files for definitions and declarations.

The rules on this page apply to every sketch in the repository, including the library examples in `libraries/SensEdu/examples/`.

### Indentation

Unlike standard Arduino code, SensEdu uses 4-space indentation. Please include the file `.theia/settings.json` with the following contents:
```json
{
  "editor.tabSize": 4
}
```

### Sketch Skeleton

Every sketch opens with a block comment naming the sketch and describing what it does, what it prints or streams, and any additional notes the reader must know up front:

```c
/*
 * ADC_3CH_DMA_Circular
 *
 * Streams three ADC channels at 44.1 kS/s per channel over USB serial without
 * gaps, using circular DMA with a double-buffered half/full transfer.
 *
 * Data is sent as raw interleaved 16-bit binary - use the MATLAB script in
 * matlab/ to receive and plot it. The host pauses/resumes the stream with 'P' / 'S'.
 *
 * The D86 LED blinks if the board runs into an error.
 */
```

After the header, the sketch is split into sections with a banner like this:

```c
/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
```

Only `Settings`, `Setup`, `Loop` and `Functions` sections are mandatory, the rest are used when the sketch grows:

| Section | Contents |
|:--------|:---------|
| `Settings` | Tunable constants and the peripheral configuration structs |
| `Structs` | `typedef enum` / `typedef struct` state definitions |
| `Globals` | Derived constants and runtime state |
| `Declarations` | Forward declarations of all private functions |
| `Setup` | `setup()` only |
| `Loop` | `loop()` only |
| `Functions` | Private function definitions, in declaration order |

#### Settings Block

Put everything a user might want to change into the settings section. Name those values as constants and comment the non-obvious ones:

```c
/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

static const uint8_t ERROR_LED_PIN = D86;

static ADC_TypeDef* adc = ADC1;
static const uint16_t SAMPLING_RATE_PER_CH = 44100;

static const uint16_t CHANNEL_NUM_PER_ADC = 3;
static uint8_t adc_pins[CHANNEL_NUM_PER_ADC] = {A0, A1, A2};
```

#### Loop

Keep `loop()` short and non-blocking. In a finished project it usually reads as a list of steps:

```c
void loop() {
    process_command();
    process_capture();
    process_usb_transfer();
}
```

### Naming Convention

* **Macros**: `SCREAMING_SNAKE_CASE`
* **Constants**: `SCREAMING_SNAKE_CASE`
* **Variables**: `snake_case`
* **Functions**: `snake_case`
* **Structs and enum types**: `PascalCase`, with `SCREAMING_SNAKE_CASE` members

```c
#define AIR_SPEED 343
static const uint16_t ADC_MIC_NUM = 4;
static uint16_t buf;
static void process_data(uint16_t* buf);

typedef enum {
    STATE_IDLE      = 0,
    STATE_RECORDING = 1
} FwState;
```

### Error Handling

Every sketch must check the library error container. Call `check_lib_errors()` after the setup sequence and once per loop iteration. If the sketch prints to the Serial Monitor, report the code there:

```c
// Checks if the library has raised any internal errors
// Prints the error code to the Serial Monitor
void check_lib_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}
```

If the serial link is busy streaming data, blink an error LED instead, so the stream is not corrupted by log messages:

```c
// Checks if the library has raised any internal errors
// Serial is busy streaming, so the error LED is used instead
static void check_lib_errors(uint8_t error_led) {
    uint32_t lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        fatal_error(error_led);
    }
}

// Halts the system and blinks the error LED
static void fatal_error(uint8_t error_led) {
    digitalWrite(error_led, !digitalRead(error_led));
    delay(200);
}
```

Use `static_assert` to verify user settings or internal variables during compilation.

```c
static_assert(sizeof(SegmentHeader) == 20, "Unexpected SegmentHeader layout.");
```

### Host Scripts

Most projects ship a host script that receives the data over USB or Wi-Fi and processes it. MATLAB is the default, Python can also be used. Host scripts live in a `matlab/` or `python/` subfolder of the project and are named after the project.

Keep the wire-protocol constants in one block and state that they must match the firmware.

#### MATLAB

* Open with a `%%` header comment naming the file and what it does, then `clear;` and `close all;`
* Split the script into `%%` cells
* Local helper functions go at the end of the file. If several scripts need the same helper, move it into a subfolder and pull it in with `addpath(genpath('./processing/'))`

```matlab
%% Audio_Recording.m
% Host script: drive the start/stop handshake, read framed segments over
% USB CDC from Audio_Recording.ino, save WAV, plot waveform + FFT.

clear;
close all;

%% User settings
ARDUINO_PORT = 'COM16';
ARDUINO_BAUDRATE = 2000000;

%% Firmware constants
Fs = 44100;
SEGMENT_SECONDS = 30;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);
```

{: .NOTE}
The serial port stays locked if a script errors out without closing it, or if the execution has been stopped manually. This prevents any new firmware uploads. Clear the `arduino` variable in MATLAB to free up the port.

#### Python

* Open with a module docstring naming the file, what it does, and what must match the firmware
* Follow a structure similar to the MATLAB scripts
* Use [PEP 8](https://peps.python.org/pep-0008/) as a code styling reference

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
* If a topic isn't covered here, refer to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). Local rules in this document take precedence (e.g., 4-space indentation).

[repository]: https://github.com/ShiegeChan/SensEdu
[SensEdu repository]: https://github.com/ShiegeChan/SensEdu
[Pull Request]: https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request
