---
title: Projects
layout: default
parent: Contributing
nav_order: 3
---

# Project Contributions
{: .fs-8 .fw-500 .no_toc}
---

We are excited to see the innovative ways in which you are utilizing SensEdu. By submitting your project, you have the opportunity to have it listed as a showcase for SensEdu functionality and included in the main repository.
{: .fw-500}

- TOC
{:toc}

### Project Submission

1. Fork [SensEdu repository]
2. Add a new project folder in your fork `~\projects\my_project\`
3. Create an Arduino sketch `~\projects\my_project\my_project.ino`. Ensure that the sketch filename matches the folder name
   1. If you have additional files (e.g., MATLAB scripts), place them in the subfolder within your project folder, such as `~\projects\my_project\matlab\my_script.m`
4. Create a documentation for your project
   1. Create a new markdown page `~\docs\Projects\my_project.md`
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
   4. Follow [Documentation Contributions]({% link Contributing/docs.md %}) for detailed page creation guidelines
   5. Share implementation details and nice pictures of you project!
5. Commit all changes to your fork and submit a [Pull Request] (PR) to the main SensEdu [repository]

### Style Guidelines

Arduino projects are written in C++. A source file is called a "sketch", it ends with `.ino` and must be in a folder with the same name. The project folder can contain additional `.ino` files to split functionality and make the code easier to manage. You can also include traditional `.h` header files for definitions and declarations.

Unlike standard Arduino code, SensEdu uses 4-space indentation. Please include the file `.theia/settings.json` with the following contents:
```json
{
  "editor.tabSize": 4
}
```

#### Naming Convention

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
* If a topic isn’t covered here, refer to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). Local rules in this document take precedence (e.g., 4-space indentation).

[repository]: https://github.com/ShiegeChan/SensEdu
[SensEdu repository]: https://github.com/ShiegeChan/SensEdu
[Pull Request]: https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request
