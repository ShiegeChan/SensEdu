---
title: Introduction
layout: home
nav_order: 1
description: "SensEdu is an educational development shield for Arduino GIGA R1 used for RADAR and communications applications."
permalink: /
---

# Introduction
{: .fs-8 .fw-500}

Welcome to the SensEdu documentation!
{: .fs-6 .fw-400}
---

SensEdu is an educational project designed to simplify entry into RADAR and communications research.
{: .fw-500}

Based on the [STM32H747] Microcontroller and runs on the [Arduino GIGA R1 WiFi] development Board, providing industry-standard embedded development experience while remaining beginner-friendly. SensEdu is perfect for students looking for a smooth introduction to this field, offering an easy-to-understand yet highly modifiable software and hardware environment. 

This project is completely open source and comes with quick support from the development team.

{: .warning }
SensEdu is still in its early development stages.
The core functionality is available, but many additional features are in progress. If you like the project, consider [contributing]({% link Contributing/index.md %}). Any input from both experienced developers and beginners is highly appreciated!

## Features
{: .fw-500}

SensEdu includes:
{:.mb-1 .mt-3}
* MEMS microphones
* Barometric Air Pressure Sensor
* Ultrasonic Transducers

You can find detailed information about each component in the [PCB Chapter]({% link PCB/index.md %}). With these components, you can build a variety of projects, such as:
* Ultrasonic Distance Measurements
* Radio Wave Generation
* Frequency Shift Keying (FSK) communication between multiple boards
* Weather Stations

## Project Structure
{: .fw-500}

The project is divided into three main parts:

* [Shield PCB]({% link PCB/index.md %}) – Located in `pcb\`
* [Hardware Abstraction Library]({% link Library/index.md %}) – Located in `libraries\SensEdu\`
* [Full Projects]({% link Projects/index.md %}) – Located in `projects\`

Each part is thoroughly documented in this wiki, allowing you to modify both software and hardware using the provided examples.

## Acknowledgments
{: .fw-500}

SensEdu was made possible thanks to these freely available tools:
{: .mb-1}
* [Arduino] and [Arduino IDE 2.x]
* [STM32H747 Documentation] and [STM32CubeMX]
* [Jekyll] and [Just the Docs] template
* [KiCAD]

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
[STM32H747 Documentation]: https://www.st.com/en/microcontrollers-microprocessors/stm32h747-757/documentation.html
[STM32CubeMX]: https://www.st.com/en/development-tools/stm32cubemx.html
[Jekyll]: https://jekyllrb.com/
[Just the Docs]: https://github.com/just-the-docs/just-the-docs/tree/main
[STM32H747]: https://www.st.com/en/microcontrollers-microprocessors/stm32h747-757.html
[Arduino GIGA R1 WiFi]: https://docs.arduino.cc/hardware/giga-r1-wifi/
[KiCad]: https://www.kicad.org/