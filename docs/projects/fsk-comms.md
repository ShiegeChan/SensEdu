---
title: FSK Communication
layout: default
math: mathjax
parent: Projects
nav_order: 7
---

# FSK Communication
{: .no_toc .fs-8 .fw-500}
---

- TOC
{:toc}

## Introduction

Telecommunications are undeniably fascinating, yet they have historically been fraught with significant technical challenges. For decades, the field has been almost exclusively dominated by Radio Frequency (RF) and electromagnetic waves. While effective, these methods often face limitations in specidic environments, such as interference or higher attenuation (in salty water, for example). This leads to an interesting alternative: why not use ultrasound as a communication medium instead? By shifting our foucs from the electromagnetic spectrum to acoustic propagation, we can develop alternative commucation systems that are both innovative and remarkably efficient for shot-range, interference-free data exchange. 

Since we are still dealing with waves, the same fundamental principles of signal modulation apply. Traditionally, two primary methods have been established to enconde information into a carrier wave:
* **Amplitude Modulation (AM):** This technique involves varying the strenght or amplitude of the signal. While simple to implement, it is highly susceptible to noise and atmospheric interference, which can earily distort the data
* **Frequency Modulation (FM):** Instead of changing the amplitude, this method varies the frequency of the carrier wave. It offers much greater resilience against background noise and signal fading. 

Our main focus will be on the latter, given that frequency-based modulation is inherently more robust for acoustic environments, as it effectively filters out most ambient noise and ensures a more reliable decoding process. Specifically, we will implement Frequency Shift Keying (FSK) as our modulation scheme. This method relies on a fundamental wave known as the 'carrier.' In our system, we assign the bit '0' to this base frequency. To represent a bit '1,' we shift the signal to a slightly higher frequency. As illustrated in the diagram, data is transmitted by switching the frequency of the acoustic wave between these two predefined values. By sending these controlled 'trains of bits,' we can effectively encode and transmit complex data and text through ultrasonic waves.

<img src="{{site.baseurl}}/assets/images/fsk_carrier_representation.png"/>
{: .text-center}
