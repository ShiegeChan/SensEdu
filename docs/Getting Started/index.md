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

{: .note}
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

---

**Step 6**{: .text-green-200} : Open SensEdu example goinf to ..., then select arduino giga r1 board and port and try to run example. Open Serial monitor and you should see numbers running

Now you are good to go, you can create an arduino sketch, include SensEdu.h and start developing. Look into other sensedu examples to explore its capablities and projects in `projects\` folder as well.

Read through Library section for better understanding of sensedu framework.

If you still have any problems or questions, please refer to the Help section in this wiki


Now you are ready, just include "SensEdu.h" into your arduino sketch


{: .note}
If you want to start as developer and contribute to SensEdu, refer to Contributing page.

[Arduino IDE]: https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE
[SensEdu release]: https://github.com/ShiegeChan/SensEdu/releases/
[installation guide]: https://www.youtube.com/