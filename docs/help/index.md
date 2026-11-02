---
title: Help
layout: default
nav_order: 6
permalink: /help/
---

# Getting Help
{: .fs-8 .fw-500 .no_toc}
---

Stuck? This page helps you solve the problem yourself or reach us if you can't.
{: .fw-500}

- TOC
{:toc}

## Before You Ask

Please start by trying to resolve the issue on your own:
* Read the [Getting Started]({% link getting started/index.md %}) section
* Use the search bar on this wiki (at the top of the page) to find related information by keywords
* Browse the [Frequently Asked Questions (FAQs)](#faqs) below
* Search online; your exact issue may already be answered

If you have tried the above and are still stuck, it is a good time to ask for help.

## Contact Us

The best way to reach us is on GitHub:
* [Issues] — confirmed bugs and planned work
* [Discussions] — everything else, including questions and ideas

If you want to keep things private, you can contact the SensEdu team directly via <a href="mailto:contact@sensedu-shield.com">contact@sensedu-shield.com</a>.

When asking for help:
* Avoid vague questions like "it doesn't work"
* Provide info on your OS, Arduino IDE version, SensEdu shield revision, and SensEdu library version
* Describe your hardware setup (wiring, sensors, power source)
* Tell us what you have already tried
* List the steps required to reproduce your issue
* Paste error messages as text and add screenshots if helpful

Your details will help us to answer your question!

Opening a new discussion on GitHub looks like this:

<img src="{{site.baseurl}}/assets/images/discussions.png" alt="Discussions page of the SensEdu repository on GitHub"/>

## FAQs

Here you can find the most frequently asked questions.

### How do I order the shield?
{: .no_toc}
Email us at <a href="mailto:contact@sensedu-shield.com">contact@sensedu-shield.com</a>.

### Why does my board not show up in the port list?
{: .no_toc}
Make sure you are using a USB cable that supports data transfer, and that the *Arduino Mbed OS Giga Boards* package is installed (see [Getting Started]({% link getting started/index.md %})). If the port is still missing, double-tap the reset button on the GIGA R1 to enter bootloader mode and select the port again. If it still fails, most likely your USB driver is installed incorrectly. Usually it ships together with Arduino IDE, so try to reinstall it. Ensure you have administrator rights during your installation.

### Why do I get `SensEdu.h: No such file or directory`?
{: .no_toc}
The library is not installed, or not where the Arduino IDE expects it. If you are not developing the library itself, install it through the *Library Manager* as described in [Getting Started]({% link getting started/index.md %}).

If you install the library manually, place the contents of the library folder from the [latest release][SensEdu release] into your Arduino libraries folder:
* Windows: `C:\Users\{username}\Documents\Arduino\libraries\`
* Linux: `/home/{username}/Arduino/libraries/`
* macOS: `/Users/{username}/Documents/Arduino/libraries/`

Restart the Arduino IDE afterwards.

### Why does the SensEdu library fail to compile?
{: .no_toc}
Usually it means that an older version of the library is still left over on your system, typically reported as an unknown type or macro:

```text
Compilation error: ISO C++ forbids declaration of 'SENSEDU_DSP_FIR_STATE_BUFFER' with no type [-fpermissive]
```

The Arduino IDE only looks for libraries in the `libraries\` folder of the sketchbook location set in *File → Preferences → Sketchbook location*, which defaults to `Documents\Arduino\`. Libraries installed through the IDE are downloaded there as well, so a repository clone kept anywhere else is ignored and the older installed copy is compiled instead.

To build against a development version, either point *Sketchbook location* at your SensEdu repository folder, or copy the development version of the library into the default `libraries\` folder, replacing the installed one.

### How can I contribute?
{: .no_toc}
Start with the [Contributing Guide]({% link contributing/index.md %}) and create a [discussion][Discussions] on GitHub for your contribution, where we will discuss your ideas and help you get started!


[Discussions]: https://github.com/ShiegeChan/SensEdu/discussions
[Issues]: https://github.com/ShiegeChan/SensEdu/issues
[SensEdu release]: https://github.com/ShiegeChan/SensEdu/releases/