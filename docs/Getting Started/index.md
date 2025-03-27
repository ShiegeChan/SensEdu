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

{: .NOTE}
*If you prefer video tutorials, we have an [installation guide] on YouTube (not yet).*

**Step 1**{: .text-green-200} : Download and Install [Arduino IDE]

**Step 2**{: .text-green-200} : Open the *Boards Manager* tab, search for `giga` and install the latest version of *Arduino Mbed OS Giga Boards* package

<img src="{{site.baseurl}}/assets/images/boards_install.png" alt="drawing"/>
{: .text-center}

**Step 3**{: .text-green-200} : Download the latest [SensEdu release]

<img src="{{site.baseurl}}/assets/images/release_install.png" alt="drawing"/>
{: .text-center}

**Step 4**{: .text-green-200} : Unzip downloaded files to a preferred location on your system

<video muted controls playsinline>
    <source src="{{site.baseurl}}/assets/videos/install_unzip.mp4" type="video/mp4">
    Video is broken.
</video>

**Step 5**{: .text-green-200} : Move extracted contents of `library` folder to the Arduino folder `My Documents\Arduino\libraries\`

<video muted controls playsinline>
    <source src="{{site.baseurl}}/assets/videos/install_libs.mp4" type="video/mp4">
    Video is broken.
</video>

**Step 6**{: .text-green-200} : Navigate to `File → Examples → SensEdu` and select `Blink_Delay` example

**Step 7**{: .text-green-200} : Select `Arduino Giga R1` board and corresponding COM port

**Step 8**{: .text-green-200} : Click **Upload** to compile and upload the sketch to your board. If the built-in LED starts blinking, the setup was successful!

<img src="{{site.baseurl}}/assets/images/select_install.png" alt="drawing"/>
{: .text-center}

<img src="{{site.baseurl}}/assets/images/demo_blink.gif" alt="drawing" width="507"/>
{: .text-center}

## Post-Setup

Now that your setup is complete, you can start developing your projects with SensEdu! Here are some next steps to get started:
* Explore the [Library]({% link Library/index.md %}) wiki section for detailed examples of each peripheral
* Check out sample projects in `projects\` folder, and respective [Projects]({% link Projects/index.md %}) section
* For troubleshooting or guidance, visit the [Help]({% link Help/index.md %}) section

{: .note}
If you are interested in contributing to SensEdu, visit the [Contributing]({% link Contributing/index.md %}) page and join our community!

[Arduino IDE]: https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE
[SensEdu release]: https://github.com/ShiegeChan/SensEdu/releases/
[installation guide]: https://www.youtube.com/