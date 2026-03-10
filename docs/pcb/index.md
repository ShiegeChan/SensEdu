---
title: PCB
layout: default
nav_order: 3
permalink: /pcb/
---

# PCB Design
{: .fs-8 .fw-500 .no_toc}
---

SensEdu focuses on the design and development of a Printed Circuit Board (PCB) tailored for the Arduino GIGA R1. The objective is to create a versatile and efficient platform capable of accommodating a wide range of ultrasound applications, ensuring optimal performance and ease of integration.
{: .fw-500}

- TOC
{:toc}

## Overview
This shield extends the Arduino Giga’s capabilities for ultrasound applications, including:

- 4-channel MEMS microphone array ([Infineon IM73A135V01](https://www.infineon.com/assets/row/public/documents/24/49/infineon-im73a135-datasheet-en.pdf)) for signal acquisition
- Ultrasound transmitters ([Prowave 328ST160](http://www.prowave.com.tw/pdf/T328S16.PDF)) for generating 32.8 kHz signals
- Multi-voltage LDOs for stable digital power and symmetric transmitter driving
- [DPS368 barometric sensor](https://www.infineon.com/assets/row/public/documents/24/49/infineon-dps368-datasheet-en.pdf) for environmental compensation
- [SHT35-DIS-B temperature and humidity sensor](https://www.infineon.com/assets/row/public/documents/24/49/infineon-dps368-datasheet-en.pdf) for environmental compensation
- Flexible output routing via digital switches

## Schematics
 <a href="{{site.baseurl}}/assets/pdf/sensedu_schematic.pdf" target="_blank">Download the PDF</a>

## Software

<span class="obj-default-theme">
  <img src="{{site.baseurl}}/assets/images/kicadlogo.jpg" alt="KiCad logo" width="200">
</span>

<span class="obj-dark-theme">
  <img src="{{site.baseurl}}/assets/images/kicadinv.png" alt="KiCad logo" width="200">
</span>

The PCB for this project was designed using [KiCad](https://kicad.org/) in Version 9.0 , an open-source software suite for electronic design automation (EDA). KiCad offers a comprehensive set of tools for schematic capture, PCB layout, and 3D visualization.

{: .NOTE}
The symbols and footprints are integrated into the project with internal project libraries. If you encounter an error, make sure that these are correctly referenced.

## PCB Layout
4-Layer Stackup:
1. **Top Layer**: Signal routing for components with ground plane fill
2. **Layer 2**: Solid ground plane 
3. **Layer 3**: Power planes (3.3V, 2.5V, 1.25V)
4. **Bottom Layer**: Signal routing for components with ground plane fill

**Front side**
{: .mb-1}

<img src="{{site.baseurl}}/assets/images/sensedu_front_bg.png" alt="drawing"/>
{: .text-center .mt-0}

**Back side**
{: .mb-1}

<img src="{{site.baseurl}}/assets/images/sensedu_back_bg.png" alt="drawing"/>
{: .text-center .mt-0}

## Power Supply

#### Key Components:
{: .no_toc}
- **Primary supply IC ([Texas Instruments LM27762DSST](https://www.ti.com/lit/ds/symlink/lm27762.pdf))**:
  - Converts 5V (Arduino) to +3.3V and -3.3V for the shield

- **Secondary LDOs**:
  - **[LDLN025M25R](https://www.st.com/resource/en/datasheet/ldln025.pdf)**: Provides 2.5V for microphone supply
  - **[TS3312AQPR](https://www.st.com/resource/en/datasheet/ts33.pdf)**: Provides 1.25V reference for amplifier offset
  

#### Power Paths:
{: .no_toc}

<script type="module">
  import mermaid from "https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs";
  mermaid.initialize({
    startOnLoad: true,
    theme: "base",
    flowchart: { curve: "linear" },
    themeVariables: {
      fontFamily: "ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', monospace",
      fontSize: "13px",
      background: "#ffffff",

      primaryColor: "#f8fafc",
      primaryTextColor: "#0f172a",
      primaryBorderColor: "#334155",

      lineColor: "#334155"
    }
  });
</script>

<div class="mermaid">
flowchart LR
  A([Arduino<br/>5V]) --> B[LM27762<br/>Charge Pump+LDO]

  B --> P[[+3.3V rail]]
  B --> N[[-3.3V rail]]

  P --> V25[[+2.5V rail]]
  P --> V125[[+1.25V rail]]

  %% Node styling
  classDef src fill:#f1f5f9,stroke:#334155,stroke-width:1.5px,rx:6,ry:6;
  classDef ic  fill:#fff7ed,stroke:#9a3412,stroke-width:1.5px,rx:6,ry:6;
  classDef pos fill:#eff6ff,stroke:#1d4ed8,stroke-width:1.5px,rx:6,ry:6;
  classDef neg fill:#fef2f2,stroke:#b91c1c,stroke-width:1.5px,rx:6,ry:6;

  class A src;
  class B ic;
  class P,V25,V125 pos;
  class N neg;

  %% Link styling (order = appearance)
  linkStyle 0 stroke:#334155,stroke-width:2px;
  linkStyle 1 stroke:#1d4ed8,stroke-width:2px;
  linkStyle 2 stroke:#b91c1c,stroke-width:2px;
  linkStyle 3 stroke:#1d4ed8,stroke-width:2px,stroke-dasharray:5 4;
  linkStyle 4 stroke:#1d4ed8,stroke-width:2px,stroke-dasharray:5 4;
</div>

<style>
  /* Make it look “diagram-like” and responsive */
  .mermaid { max-width: 900px; }
  .mermaid svg { width: 100%; height: auto; }
</style>

#### Indicators:
{: .no_toc}
- Green LED lights when 3.3V is active


## MEMS Microphones ([Infineon IM73A135V01XTSA1](https://www.infineon.com/assets/row/public/documents/24/49/infineon-im73a135-datasheet-en.pdf))

The Infineon IM73A135V01XTSA1 is a high-performance analog MEMS microphone designed for precise sound capture. It is particularly suitable for ultrasound applications due to its high Signal-to-Noise Ratio (SNR) and low Total Harmonic Distortion (THD).


  <img src="{{site.baseurl}}/assets/images/component_mic.png" alt="drawing" width="250"/>

- **Quantity**: 4
- **Key Features**:
  - **SNR**: 73 dB, which is excellent for capturing low-level ultrasound signals with high fidelity
  - **Frequency Response**: Wide and flat frequency response suitable for ultrasound frequencies
  - **Sensitivity**: -38 dBV/Pa, enabling detection of weak ultrasound signals
  - **Low Power Consumption**: Efficient power usage, critical for battery-operated or low-power applications
- **Power Supply**: 
  - 2.5V supply from the LDLN025M25R LDO


## Amplifier
- **Instrumentation Amplifier ([AD8426ACPZ](https://www.analog.com/media/en/technical-documentation/data-sheets/ad8426.pdf))**:
  
The AD8426ACPZ is a high-performance instrumentation amplifier from Analog Devices. It is designed for accurate, low-noise amplification of small differential signals, making it ideal for amplifying the weak signals from the MEMS microphones in ultrasound applications.

- **Key Features**:
  - **Low Noise**: 1 nV/√Hz noise density, ensuring minimal introduction of noise into the signal
  - **High Common-Mode Rejection Ratio (CMRR)**: Ensures that noise from the power supply or other sources is minimized
  - **Single-Supply Operation**: 3.3V supply voltage, compatible with the overall system design
  - **Configurable Gain**: Gain can be set using external resistors, with a typical configuration providing a gain of 50 (using a 1kΩ resistor)
- **Usage in Design**:
  - **Microphone Signal Amplification**: The AD8426 amplifiers are used to boost the small amplitude signals from the MEMS microphones (IM73A135V01XTSA1) to levels that fully utilise the range of the internal ADCs
  - **Configuration**: 4x AD8426 amplifiers provide four channels of amplification, each with a gain of 50. Two of them are  for the Infineon IM73A135V01XTSA1 microphones and two for external sources
  - **Amplifier for external signals**:  ***J8/J9/J10/J11***: 4-pin headers for connecting external microphones or other signal sources
    - Pin 1: 2.5V (mic supply)
    - Pin 2: Amplifier input +
    - Pin 3: Amplifier input -
    - Pin 4: GND
  - **Power Supply**: Operates with single 3.3V supply

### Analog Input Connections

| Input | Amplifier Channel | Arduino Pin | STM32 Pin | Available ADCs |
|:------|:--------|:------|:------|:------------|
| MK1    | U4 CH1 | A0    | PC4_C | ADC1 & ADC2 |
| MK2   | U4 CH2  | A1    | PC5_C | ADC1 & ADC2 |
| MK3   | U5 CH1  | A2    | PB0_C | ADC1 & ADC2 |
| MK4   | U5 CH2  | A3    | PB1_C | ADC1 & ADC2 |
| J8    | U6 CH1  | A8    | PC2_C | ADC3        |
| J9    | U6 CH2  | A9    | PC3_C | ADC3        |
| J10   | U7 CH1  | A10   | PA1_C | ADC1 & ADC2 |
| J11   | U7 CH2  | A11   | PA0_C | ADC1 & ADC2 |


## I2C based Sensors

Two different sensors are used to gain additional information about the environment to increase the accuracy of the measurements. The communication with these is implemented by I2C bus.
I2C2 (D20 and D21) corresponds to the correct instance of the Arduino.

| Sensor | Address |
|:---|:---|
| DPS368   | 0x77 |
| SHT35-DIS   | 0x44 | 

{: .NOTE}
Use Wire2 to be able to talk to the sensors via I2C2 of the Arduino Giga.


### Barometric Pressure Sensor ([Infineon DPS368](https://www.infineon.com/assets/row/public/documents/24/49/infineon-dps368-datasheet-en.pdf))

 The DPS368 is a high-precision barometric pressure sensor designed by Infineon. It measures both pressure and temperature, providing essential environmental data that can be used to compensate for variations in ultrasound propagation due to changes in atmospheric conditions.

 <img src="{{site.baseurl}}/assets/images/DPS.png" alt="drawing" width="250"/>
  {: .text-left}

   - **Interface**: I2C 
   - **Key Features**:
     - **Pressure Range**: 300 to 1200 hPa, suitable for most environmental conditions
     - **Temperature Range**: -40 to 85°C, allowing for robust operation in various environments
     - **High Accuracy**: Pressure accuracy of ±1 hPa and temperature accuracy of ±0.5°C, ensuring precise environmental compensation
     - **Low Power Consumption**: Ideal for continuous monitoring without significant power drain
     - **Power Supply**: Operates with single 3.3V supply  
  - **Usage**: 
     - The DPS368 is used to measure atmospheric pressure and temperature, which can then be used to adjust the ultrasound measurements for more accurate distance and velocity calculations


### Humidity and Temperature Sensor ([Sensirion SHT35-DIS](https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf))

  The SHT35-DIS is a high-precision humidity and temperature sensor designed by Sensirion. It measures both humidity and temperature, providing essential environmental data with a short start-up time.

   <img src="{{site.baseurl}}/assets/images/SHT4.png" alt="drawing" width="250"/>


   - **Interface**: I2C 
   - **Key Features**:
     - **Pressure Range**: 0 to 100% relative humidity(RH)
     - **Temperature Range**: -40 to 120°C, allowing for robust operation in various environments
     - **High Accuracy**: Humidity accuracy of ±1.5% RH and temperature accuracy of ±0.2°C
     - **Low Power Consumption**: Ideal for continuous monitoring without significant power drain
     - **Power Supply**: Operates with single 3.3V supply  
   - **Usage**: 
      - The SHT35-DIS is used to measure relative humidity and temperature, which can then be used to create a weather station
  




## Ultrasound Transmitters

### Speakers ([Prowave 328ST160](http://www.prowave.com.tw/pdf/T328S16.PDF))
   
The Prowave 328ST160 is an ultrasound speaker designed specifically for emitting high-frequency sound waves. With a resonance frequency of 32.8 kHz, it is ideal for generating ultrasound signals that can be used in various applications such as distance measurement and object detection.

<img src="{{site.baseurl}}/assets/images/speaker.png" alt="drawing" width="250"/>


- **Key Features**:
  - **Resonance Frequency**: 32.8 kHz, which is optimal for many ultrasound applications
  - **High Sound Pressure Level (SPL)**: Ensures strong and consistent ultrasound signal transmission
  - **Directional Beam Pattern**: Focuses the ultrasound waves, improving accuracy in detecting reflections from objects
  - **Durability**: Designed to withstand environmental factors, making it suitable for both indoor and outdoor applications
- **Integration in Design**:
  - **Driving Circuit**: The ultrasound speakers are driven by the ADA4511-2 operational amplifier, which provides the necessary gain and power to achieve optimal performance
  - **2x Speakers (LS1&LS2)**
  - **Signal Input Options**: 
    - **Analog Waveforms**: Provided through Arduino DAC pins (D84 and D85)
    - **PWM Signals**: Filtered through an RC network (4.7kΩ resistor and 1nF capacitor → cutoff ~34 kHz) to achieve a smooth signal
  - **Power Supply**: Operates with dual 3.3V supply

### Operational Amplifier ([ADA4511-2ARMZ](https://www.analog.com/media/en/technical-documentation/data-sheets/ADA4891-1_4891-2_4891-3_4891-4.PDF))

  The ADA4511-2 is a dual-channel, high precision, low-noise, rail-to-rail operational amplifier from Analog Devices. It is designed for applications requiring fast and precise signal amplification, making it suitable for driving the ultrasound transmitters (Prowave 328ST160).

- **Key Features**:
  - **High Slew Rate**: Ensures rapid response to input signal changes, critical for generating clean ultrasound signals
  - **Low Noise**: Low input noise density, providing a clean amplification of the input signal
  - **Configurable Gain**: Gain can be set using external feedback resistors, with the current configuration providing a gain of 2 (using a 22kΩ feedback resistor and a 22kΩ resistor to ground)
- **Usage in Design**:
  - **Driving Ultrasound Transmitters**: The ADA4511-2 amplifiers are used to symmetrically drive the Prowave 328ST160 speakers, providing the necessary power and precision to generate effective ultrasound signals
  - **Power Supply**: Operates with dual 3.3V supply



## Digitally switchable connections

  The SensEdu supports switching between DAC and PWM based driving of the ultrasound transmitters. By using the NC7SB3157P6X from onSemi, the source or the signal can be digitally switched.

| Signal | Pin |Function                                           |
|:------|:----|:---------------------------------------------------|
| S1    | D6 | Switches between DAC1 (D85) and SE_PWM_1 (D4) |
| S2   | D7 | Switches between DAC0 (D84) and SE_PWM_3 (D48) |

{: .WARNING}
SE_PWM_3 is available only with 2x18 housing, which is by default not populated.


[link1]: https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax
[link2]: https://just-the-docs.com/
[Infineon IM73A135V01XTSA1]: https://www.infineon.com/dgdl/Infineon-IM73A135-DataSheet-v01_00-EN.pdf?fileId=8ac78c8c7f2a768a017fadec36b84500