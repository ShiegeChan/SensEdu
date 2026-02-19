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

_Binary FSK signal generation_
{: .text-center}

### Goertzel Algorithm

To decode these frequency shifts, we require an efficient method to detect specific tones within a continuous stream of acoustic data. While the Fast Fourier Transform (FFT) is the most common tool for spectral analysis, it is computationally expensive because it analyzes the entire frequency spectrum at once.

Instead, our system utilizes the Goertzel Algorithm. Unlike the FFT, Goertzel is a digital filter that focuses exclusively on pre-selected frequencies. In our case, as the resonance frequency of our transducers is around 33 kHz, we have chosen 31 kHz and 35 kHz tones. This selection provides us with enough bandwidth to detect each tone without interference, making the algorithm significantly faster and more resource-efficient for embedded systems. By calculating the energy of only these two specific frequency components, the SensEdu board can determine in real-time whether a bit '0' or a bit '1' is being received, effectively filtering out ambient noise and ensuring high-speed, reliable communication.

To implement this algorithm, we separate it into a specialized two-stage digital filter. The first stage is a second-order **IIR filter** that process the ADC input sequence **x[n]** to calculate an intermediate value **s[n]**. This is highly efficient as it only requires one multiplication per sample:

$$
\begin{equation}
s[n] = x[n] + 2 \cos(\omega_0) \cdot s[n - 1] - s[n - 2]
\label{eq:goertzel}
\end{equation}
$$

Where s[-1] = s[-2] = 0 at the start of each bit window. After processing the 
 samples, the second stage (a **FIR filter**) computes the final power of the frequency components:

$$
\begin{equation}
Power = s[n]^2 + s[n - 1]^2 - s[n] \cdot s[n - 1] \cdot 2\cos(\omega_0)
\label{eq:power}
\end{equation}
$$

The system then compares the resulting Power values, decoding the bit as '0' or '1' based on the frequency component which exhibits the highest energy.

## Hardware set up

To perform this modulated communication, the essential components required are a transmitter and a receiver. For this project, we use two SensEdu boardes based on the Arduino GIGA R! WIFI:  

* **Transmitter (Tx):** Uses an ultrasonic **tranducer**{: .text-green-000} to generate and emit the modulated acoustic waves.

* **Receiver (Rx):** Captures the incoming signal using the onboard **MEMS microphones**{: .text-green-000}, which provides the high sensitivity needed for ultrasonic frequencies.

<img src="{{site.baseurl}}/assets/images/two_boards_fsk.jpeg" width="500"/>
{: .text-center}

The two boards are placed directly in front of each other to ensure a direct transmission path and to minimize signal attenuation caused by the environment. For our tests, they were separated by a distance of 40 to 50 cm, providing a stable channel for data exchange.


## Software implementation

### LUT Generator (MATLAB)

To ensure high-fidelity signal generation without taxing the Arduino’s CPU during transmission, the waveforms for the bit '0' and bit '1' were pre-calculated using a MATLAB script.

This LUT Generator produces two arrays of 12-bit integers, tailored to the DAC’s resolution and the specific frequencies of 31 kHz and 35 kHz. By pre-computing these sine waves, the transmitter only needs to cycle through the arrays, significantly reducing real-time computational overhead and ensuring a stable, jitter-free carrier signal. 









