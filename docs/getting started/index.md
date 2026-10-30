---
title: Getting Started
layout: default
nav_order: 2
permalink: /getting-started/
---

# Getting Started
{: .fs-8 .fw-500 .no_toc}
---

Follow these steps to begin developing your projects with SensEdu.
{: .fw-500}

- TOC
{:toc}

## Basic Setup

**Step 1**{: .text-green-200} : Mount SensEdu Shield onto the Arduino GIGA R1.

**Step 2**{: .text-green-200} : Connect Arduino to PC with a USB-C cable

**Step 3**{: .text-green-200} : Download and Install [Arduino IDE]

**Step 4**{: .text-green-200} : Open the *Boards Manager* tab, search for `giga` and install the latest version of *Arduino Mbed OS Giga Boards* package

<img src="{{site.baseurl}}/assets/images/boards-install.png" alt="drawing"/>
{: .text-center}

**Step 5**{: .text-green-200} : Open the *Library Manager* tab, search for `SensEdu` and install the latest version of the *SensEdu Library*

<img src="{{site.baseurl}}/assets/images/sensedu-lib-install.png" alt="drawing"/>
{: .text-center}

**Step 6**{: .text-green-200} : Navigate to `File → Examples → SensEdu` and select `Blink_Delay` example

**Step 7**{: .text-green-200} : Select `Arduino Giga R1` board and corresponding COM port

**Step 8**{: .text-green-200} : Click **Upload** to compile and upload the sketch to your board. If the built-in LED starts blinking, the initial setup was successful! From here you can explore the rest of the examples, most of them are walked through in the [Library]({% link library/index.md %}) section.

<img src="{{site.baseurl}}/assets/images/select_install.png" alt="drawing"/>
{: .text-center}

<img src="{{site.baseurl}}/assets/images/demo_blink.gif" alt="drawing" width="507"/>
{: .text-center}

## Projects Setup

After completing the basic SensEdu setup, you can explore already established projects.

{: .NOTE}
Most projects pair an Arduino sketch with a host script. The walkthrough below uses [MATLAB](https://www.mathworks.com/products/matlab.html), so make sure it is installed. Some other projects also ship a Python alternative.

**Step 1**{: .text-green-200} : Download the latest [SensEdu release]

<img src="{{site.baseurl}}/assets/images/release_install.png" alt="drawing"/>
{: .text-center}

**Step 2**{: .text-green-200} : Unzip downloaded files to a preferred location on your system

<video muted controls playsinline>
    <source src="{{site.baseurl}}/assets/videos/install_unzip.mp4" type="video/mp4">
    Video is broken.
</video>

**Step 3**{: .text-green-200} : Open `.\projects\Ultrasonic_Ranging\complete_ultrasonic_ranging\complete_ultrasonic_ranging.ino` and upload the sketch (the same way you uploaded the blinky example). This is the [Pulse-Echo Ranging]({% link projects/pulse-echo-ranging.md %}) project

**Step 4**{: .text-green-200} : Open `.\projects\Ultrasonic_Ranging\complete_ultrasonic_ranging\matlab\complete_data_acquisition.m`

**Step 5**{: .text-green-200} : Update COM port of the host script according to Arduino IDE port

**Step 6**{: .text-green-200} : Run the host script

{: .NOTE}
If the host script does not do anything, usually it is because the COM port is busy. Typically it is caused by the Serial Monitor being open in the Arduino IDE - close it.

**Step 7**{: .text-green-200} : Test with a flat object on top of the board, you should see the distance to the object.

## Post-Setup

Now that your setup is complete, you can start developing your projects with SensEdu! Here are some next steps to get started:
* Explore the [Library]({% link library/index.md %}) wiki section for detailed examples of each peripheral
* Check out other sample projects in `projects\` folder, and respective [Projects]({% link projects/index.md %}) section
* For troubleshooting or guidance, visit the [Help]({% link help/index.md %}) section

{: .NOTE}
If you are interested in contributing to SensEdu, visit the [Contributing]({% link contributing/index.md %}) page and join our community!

[Arduino IDE]: https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE
[SensEdu release]: https://github.com/ShiegeChan/SensEdu/releases/