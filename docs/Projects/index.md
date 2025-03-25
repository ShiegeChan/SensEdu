---
title: Projects
layout: default
nav_order: 5
permalink: /projects/
---

# Projects developed with SensEdu
{: .fs-8 .fw-500}
---

The following section documents select projects developed by SensEdu contributors. These real-world examples demonstrate practical use cases of the platform and highlight its capabilities.
{: .fw-500}

{: .warning}
Currently, this section is the main focus of our team. We aim to expand our collection of polished, fully documented projects to better illustrate SensEdu applications. Contributions are highly welcomed! If you’ve created a nice project, please consider adding it to both this wiki and our main repository by following [these guidelines]({% link Contributing/projects.md %}).

{: .note}
Most of the projects utilize SensEdu library, but it is not explicitly necessary, you can develop projects with standard or other custom Arduino GIGA R1 libraries.

As detailed in the [PCB specifications]({% link PCB/index.md %}), all applications center around three core hardware components:
* MEMS microphones
* Barometric Air Pressure Sensor
* Ultrasonic Transducers

### Ultrasonic Transceiver
SensEdu’s primary functionality lies in its *ultrasonic transceiver* capabilities. The system pairs 32 kHz ultrasonic transmitters (speakers) with high-sensitivity MEMS microphones optimized for 10-100 kHz reception (32 kHz resonant frequency). This combination enables implementations such as:
* Distance measurements
* Collision avoidance systems
* Robotic navigation
* Radars
* Radio communication

<img src="{{site.baseurl}}/assets/images/UltrasonicBat.png" alt="drawing"/>
{: .text-center .mb-1}

Biological inspiration: Bat echolocation using ultrasonic waves (Image source: [BBC Science Focus]).
{: .mt-1}

<img src="{{site.baseurl}}/assets/images/UltrasonicDrone.png" alt="drawing"/>
{: .text-center .mb-1}

Modern application: Drone obstacle detection systems (Image source: [Microcontrollertips]).
{: .mt-1}

### Barometric Pressure Sensor
Moreover, the board is equipped with a *barometric sensor* which enables atmospheric measurements. It could be utilized for:
* Elevation tracking (climbing\descent detection)
* Environmental monitoring systems
* Motion pattern recognition (e.g., falling)
* Robot vertical navigation

[BBC Science Focus]: https://www.sciencefocus.com/nature/how-does-echolocation-work
[Microcontrollertips]: https://www.microcontrollertips.com/principle-applications-limitations-ultrasonic-sensors-faq/