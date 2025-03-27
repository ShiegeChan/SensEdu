---
title: FMCW Radar
layout: default
math: mathjax
parent: Projects
nav_order: 3
---

# FMCW Radar
{: .no_toc .fs-8 .fw-500}
---

{: .WARNING}
This project is currently in its initial development phase and is subject to significant changes, updates, and improvements. The project enables a rudimentary implementation of an FMCW radar and requires two Arduino boards with SensEdu Shields.

The **FMCW Radar Project**{: .text-green-000} utilizes the **SensEdu Shield**{: .text-green-000} to perform **distance measurements**{: .text-green-000} using FMCW radar techniques.
{:.fs-5}

FMCW radars are widely used for short range applications like automotive or drone ranging. These radars have the advantage of providing good spatial resolution while being simple to implement.

## Table of contents
{: .no_toc .text-delta }
1. TOC
{:toc}

## FMCW Radar Principle
{: .text-yellow-300}
This chapter explains the fundamentals of FMCW radars in order to better understand their magic. For more in-depth information, I would recommend [Marshall Bruner's YouTube channel](https://www.youtube.com/@MarshallBrunerRF) — his channel is a gem on the subject! The animated graphs on this doc have been created with the [Manim Community](https://docs.manim.community/en/stable/index.html) python library and are greatly inspired by Marshall Bruner's code.

### Chirp Signal
{: .text-yellow-100}
FMCW radars involves the continuous transmission and reception of a frequency modulated signal also known as chirp. We will call the transmitted signal <span style="color:#57c2da !important">$$T_x$$</span>{: .text-blue-000} and we are using a sawtooth modulation. The sawtooth modulation linearly sweeps a bandwidth $$B$$ over the chirp period $$T_c$$.

The transmitted signal is reflected from a static object and received by the radar. The received signal is called <span style="color:#9c76ab !important">$$R_x$$</span>. Below is a representation of $$T_x$$ and $$R_x$$'s amplitude and frequency over time :

<video autoplay loop>
  <source src="{{site.baseurl}}/assets/videos/TxRx.mp4" type="video/mp4">
</video>
{: .text-center}

$$T_x$$ and $$R_x$$'s Amplitude and Frequency as a Function of Time
{: .text-center}

As we see, $$R_x$$ is exactly the same signal as $$T_x$$ but delayed by $$t_0$$, the time of flight (TOF). We can intuitively establish a relationship between $$t_0$$ and the distance to the object :

<div id="eq1" class="fs-5 text-center">
  $$t_0 = \frac{2d}{c} \tag{1}$$
</div>

where
- $$c = 343 \, \text{m} \cdot \text{s}^{-1}$$ the speed of sound in air $$(T = 293 \, K)$$
- $$d$$ is the distance to the object $$\text{(m)}$$

### Distance
{: .text-yellow-100}
Where pulse radars measure time and TOF to evaluate distance, FMCW radars measure frequencies! The TOF $$t_0$$ cannot be estimated directly since we are sending waves continuously but we have another valuable information : The beat frequency $$f_b$$.

The beat frequency is defined as the frequency equal to the difference between two sinusoids. $$f_b$$ appears in the figure below :

<img src="{{site.baseurl}}/assets/images/FBeat.png" alt="drawing" width="800"/>
{: .text-center}

The Beat Frequency $$f_b$$ appears due to the slight delay between $$T_x$$ and $$R_x$$**
{: .text-center}

The geometrical approach is the simplest way to understand how to derive the distance from $$f_b$$. Let's define the slope of the chirp $$s$$ being $$s=\frac{B}{T_c}$$. Using the last figure and Eq [(1)](#eq1) we now have the following relationship :

<div id="eq2" class="fs-5 text-center">
  $$f_b = s t_0 = s \frac{2d}{c} \tag{2}$$
</div>

Thus, the distance can easily be derived :

<div id="eq3" class="fs-5 text-center">
  $$\fcolorbox{red}{}{$\displaystyle d = \frac{c f_b}{2s}$} \tag{3}$$
</div>

We know how to compute the distance, that's great! But how do we extract $$f_b$$ ...

### Beat frequency
To extract the beat frequency we need to mix the $$T_x$$ and $$R_x$$. Mixing two signals is essentially multiplying them but it enables us to subtract in the frequency domain.

If we freeze $$T_x$$ and $$R_x$$ at a given time, they're basically two sinusoids at different frequencies. Let's define $$f_T$$ and $$f_R$$ the instantaneous frequencies of our two signals where $$f_T>f_R$$ (the phase of the signals is not relevant here). We also define $$y_{mix}$$ the mixed signal.

At a given time, $$y_{mix}$$ is expressed as

<div id="eq4" class="fs-5 text-center">
  $$ 
  y_{mix} = T_x \cdot R_x = \sin(2 \pi f_{T} t) \cdot \sin(2 \pi f_{R} t) \\
  = \frac{1}{2}\big[\cos\big(2 \pi \textcolor{#7FFF00}{\underbrace{(f_T-f_R)}_{f_b}} t\big) + \cos\big(2 \pi \textcolor{#008000}{\underbrace{(f_{T}+f_{R})}_{\text{HF component}}} t\big)\big] \tag{4}
  $$
</div>

The mixing operation produces a signal which is the sum of two sinusoids :
- One at the frequency $$f_{T}-f_{R}$$
- One at the frequency $$f_{T}+f_{R}$$

The high frequency component HF at $$f_{T}+f_{R}$$ can easily be removed with a low pass filter. The remaining signal is a simple sinudoid at the beat frequency $$f_b=f_{T}-f_{R}$$. Below is a representation of the mixing signal operation :

<video autoplay loop>
  <source src="{{site.baseurl}}/assets/videos/Mixing.mp4" type="video/mp4">
</video>
{: .text-center}

$$y_{mix}$$ is the product of $$T_x$$ and $$R_x$$
{: .text-center}

### Distance Resolution & Max Range 

Distance resolution and max range are two critical parameters regarding radar performance. Distance resolution defines the smallest distance at which a radar can separate two objects. The distance resolution $$\Delta d$$ is defined by

<div id="eq5" class="fs-5 text-center">
  $$\fcolorbox{red}{}{$\displaystyle \Delta d = \frac{c}{2B}$} \tag{5}$$
</div>

where

- $$c$$ is the speed of sound $$\text{(m} \cdot \text{s}^{-1})$$

- $$B$$ is the bandwidth of the chirp  $$\text{(Hz)}$$

{: .IMPORTANT}
Distance resolution is only dependent on the radar's bandwidth !

For ultrasonic FMCW radars, the maximum range $$d_{max}$$ is heavily limited by $$T_c$$. The maximum range $$d_{max}$$ is defined by

<div id="eq6" class="fs-5 text-center">
  $$\fcolorbox{red}{}{$\displaystyle d_{\text{max}} = c \frac{T_c}{2}$} \tag{6}$$
</div>

where

- $$c$$ is the speed of sound $$\text{(m} \cdot \text{s}^{-1})$$

- $$T_c$$ is the period of the chirp  $$\text{(s)}$$

{: .IMPORTANT}
For ultrasonic FMCW radars, the max range bottleneck is $$T_c$$. For regular FMCW radars, it's usually the ADC sampling frequency.


## Radar Design

{: .WARNING}
Due to mechanical coupling issues on the current SensEdu Shield, this radar implementation will not be able to perform distance measurements with a single SensEdu Shield. The implementation requires two Arduino boards with SensEdu Shields and the radar measures the distance between the two boards.

The following block diagram illustrates the architecture of the FMCW radar :

<img src="{{site.baseurl}}/assets/images/RadarDesign.png" alt="drawing" width="800"/>
{: .text-center}

FMCW Radar Block Diagram
{: .text-center}

The chirp signal is generated in the transmitter shield and converted to an analog signal with the DAC. Tx is then doubled and follows 2 paths :

- Tx is amplified and sent to the ultrasonic transducer
- Tx is sent to the receiving shield by using a jump wire between the transmitter DAC pin and the receiver ADC1 pin.

On the receiver shield, Tx goes through ADC1. The MEMS microphone receives a signal Rx which is amplified with a low noise amplifier (LNA) and goes through ADC2. This method is by no means optimal but it enables to send Tx and Rx signals to MATLAB from the same shield which simplifies signal synchronization. The Tx and Rx signals are sent from the receiver shield to MATLAB.

The following signal processing is performed in MATLAB to measure the distance between the two shields:

- Compute mixing operation between Tx and Rx
- Compute FFT of mixed signal $$y_{mix}$$
- Low-pass filter $$y_{mix}$$
- Extract beat frequency and compute distance

## Code Implementation
{: .text-yellow-300}

### Send DAC and ADC data
{: .text-yellow-100}

### Receiving the data
{: .text-yellow-100}

## Distance computation
{: .text-yellow-300}