---
title: Chirp Signal Generation
layout: default
math: mathjax
parent: Projects
nav_order: 2
---

# Chirp Signal Generation
{: .no_toc .fs-8 .fw-500}
---

The chirp signal generation project aims at proviving basic code to generate 
frequency modulated continuous waves on the SensEdu Shield using the Arduino Giga R1. The projects provides two types of frequency modulation : sawtooth and triangular.

Chirp signals are encountered in numerous fields like radar & sonar systems, telecommunications, signal processing and more. You will find more information on the potential applications from our upcoming FMCW radar project.

{: .fw-500}

## Table of contents
{: .no_toc .text-delta }
1. TOC
{:toc}

## Chirp Generation Function
Arduino does not provide any built-in chirp signal function. There are workarounds using MATLAB's built-in chirp function but our idea was to create this signal directly in Arduino with the SensEdu library.

The `generateSawtoothChirp` and `generateTriangularChirp` functions both calculate the values to generate the lookup table (LUT) of a sawtooth chirp and triangular chirp signal respectively.

```c
void generateSawtoothChirp(uint16_t* array)
```

Parameter 
* `uint16_t* array` : A pointer to an array where the generated chirp signal will be stored.




---

## Main code
The Chirp_SawtoothMod.ino and Chirp_TriangularMod.ino and


<!-- example text

[example link]

example list:
* sdsd
* sdsds
* sdsds

example list 2:
1. sdsd
2. sdsds
3. sdsds

`marked text`

**Bold text**

*Italics*

```c
// cool code
```


{. :warning}
callout #1

{. :note}
callout #1 -->


[example link]: https://github.com/ShiegeChan/SensEdu
[link1]: https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax
[link2]: https://just-the-docs.com/