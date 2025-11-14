---
title: Ultrasonic Ranging WiFi GUI
layout: default
math: mathjax
parent: Projects
nav_order: 2
---


# Ultrasonic Ranging WiFi GUI
{: .no_toc .fs-8 .fw-500}
---

{: .WARNING}
This project is in an early draft state. The WiFi GUI currently supports only raw data streaming with the provided Arduino WiFi example. Many UI elements are placeholders and non-functional.


- TOC
{:toc}


## Introduction

For the whole ultrasonic implementation info etc, refer to main [Ultrasonic Ranging page]({% link Projects/UltraSound.md %}). This page focuses on differences: WiFi implementation and GUI.

## Current status
A draft GUI (WiFi_GUI.mlapp) is now included. In its current form, the GUI works with the **UltraSound-WiFi-GUI**, but displays only raw signals. The GUI application will need to be adapted to get the full detailed readings.

Functional UI elements:
  - IP Address text field (to target the device on the network)
  - Start and Stop buttons (begin/end raw data streaming)
  - Main Axes (live plot for incoming raw data)

The other UI elements are non-functional dupes and will still have to be implemented.

TO DO: 
 - Filtered / XCORR / Distance buttons
 - CLI for outputs
 - simple / detailed data transmission
 - save plots / data
 - proper axis names
 - if the data is not received due to poor connection, don't crash the app, just skip one iteration and clean the buffers

Current draft:
![alt text]({{site.baseurl}}/assets/images/wifi_gui_draft.png)

Full planned sketch:
![alt text]({{site.baseurl}}/assets/images/gui_sketch.png)

