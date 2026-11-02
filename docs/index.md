---
title: Introduction
layout: home
nav_order: 1
description: "Ultrasonic development shield for Arduino GIGA R1, designed as an open-source educational project to lower the barrier to ultrasonic sensing and research."
permalink: /
---

# Introduction
{: .fs-8 .fw-500}

Welcome to the SensEdu documentation!
{: .fs-6 .fw-400}
---

<video autoplay loop muted playsinline>
  <source src="{{site.baseurl}}/assets/videos/animation.mp4" type="video/mp4">
  Video is broken.
</video>

SensEdu is an ultrasonic ranging and detection development shield for Arduino GIGA R1, designed as an open-source educational project to lower the barrier to ultrasonic sensing and research.
{: .fw-500}

Based on the [STM32H747] Microcontroller and runs on the [Arduino GIGA R1 WiFi] development Board, providing industry-standard embedded development experience while remaining beginner-friendly. SensEdu is perfect for students looking for a smooth introduction to this field, offering an easy-to-understand yet highly modifiable software and hardware environment. 

This project is completely open source and comes with quick support from the development team.

{: .NOTE}
If you like the project, consider [contributing]({% link contributing/index.md %}). Any input from both experienced developers and beginners is highly appreciated!

## Features
{: .fw-500}

SensEdu includes:
{:.mb-1 .mt-3}
* 4x MEMS microphones, sensitive well into the ultrasonic range
* 2x Ultrasonic transducers with a ~33 kHz center frequency
* 4x Dual-channel instrumentation amplifiers
* Barometric air pressure sensor
* Humidity & temperature sensor

You can find detailed information about each component in the [PCB Chapter]({% link pcb/index.md %}). With these components, you can build a variety of projects, such as:
* Pulse-Echo and FMCW ultrasonic ranging
* Ultrasonic 3D Tracking
* Ultrasonic chirp generation
* Doppler Velocimeter
* FSK communication
* Audio recorder
* EMG controls
* Weather Station

{: .NOTE}
Many of these are already fully implemented and documented in the [Projects]({% link projects/index.md %}) chapter.

## Where to Start
{: .fw-500}

Follow the [Getting Started]({% link getting started/index.md %}) guide to mount the shield, install the Arduino board package and the SensEdu library, and upload your first sketch. If something does not work as expected, the [Help]({% link help/index.md %}) section collects the most common issues.

## Repository Structure
{: .fw-500}

The SensEdu repository is divided into four main parts:

* [Shield PCB]({% link pcb/index.md %}) – Located in `pcb\`
* [Hardware Abstraction Library]({% link library/index.md %}) – Located in `libraries\SensEdu\`
* [Full Projects]({% link projects/index.md %}) – Located in `projects\`
* Documentation – Located in `docs\`, rendered into this wiki

Each part is thoroughly documented in this wiki, allowing you to modify both software and hardware using the provided examples.

## Acknowledgments
{: .fw-500}

SensEdu was made possible thanks to these tools and libraries:
{: .mb-1}
* [Arduino], [Arduino IDE 2.x] and the [Arduino Mbed OS Giga Core]
* [STM32H747 Documentation] and [STM32CubeMX]
* [CMSIS-DSP] for on-board signal processing
* [XENSIV™ Digital Pressure Sensor Arduino Library] and [Sensirion SHT4x Arduino Library] for the environmental sensors
* [KiCad] for the shield hardware
* [MATLAB] and [Python] for the host-side scripts
* [Manim Community] for the animated figures
* [Jekyll] and the [Just the Docs] template for this wiki

Thank You to All Contributors!
{: .fw-500 .mb-1}

<ul class="list-style-none">
{% for contributor in site.github.contributors %}
  <li class="d-inline-block mr-1">
     <a href="{{ contributor.html_url }}"><img src="{{ contributor.avatar_url }}" width="32" height="32" alt="{{ contributor.login }}"></a>
  </li>
{% endfor %}
</ul>

[Arduino IDE 2.x]: https://github.com/arduino/arduino-ide
[Arduino]: https://www.arduino.cc/
[Arduino Mbed OS Giga Core]: https://github.com/arduino/ArduinoCore-mbed
[XENSIV™ Digital Pressure Sensor Arduino Library]: https://github.com/Infineon/arduino-xensiv-dps3xx
[Sensirion SHT4x Arduino Library]: https://github.com/Sensirion/arduino-i2c-sht4x
[STM32H747 Documentation]: https://www.st.com/en/microcontrollers-microprocessors/stm32h747-757/documentation.html
[STM32CubeMX]: https://www.st.com/en/development-tools/stm32cubemx.html
[CMSIS-DSP]: https://github.com/ARM-software/CMSIS-DSP
[MATLAB]: https://www.mathworks.com/products/matlab.html
[Python]: https://www.python.org/
[Manim Community]: https://docs.manim.community/en/stable/index.html
[Jekyll]: https://jekyllrb.com/
[Just the Docs]: https://just-the-docs.com/
[STM32H747]: https://www.st.com/en/microcontrollers-microprocessors/stm32h747-757.html
[Arduino GIGA R1 WiFi]: https://docs.arduino.cc/hardware/giga-r1-wifi/
[KiCad]: https://www.kicad.org/
