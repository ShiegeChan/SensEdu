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

{: .warning}
This project is currently in its initial development phase and is subject to significant changes, updates, and improvements. The current state of the project does not represent the final working version, and some features may be incomplete.

The **FMCW Radar Project**{: .text-green-000} utilizes the **SensEdu Shield**{: .text-green-000} to perform **distance measurements**{: .text-green-000} using FMCW radar techniques.
{:.fs-5}

FMCW radars are widely used for short range applications like automotive or drone ranging. These radars have the advantage of providing good spatial resolution while being simple to implement.

## Table of contents
{: .no_toc .text-delta }
1. TOC
{:toc}

## FMCW Radar Principle
This chapter explains the fundamentals of FMCW radars in order to better understand their magic. For more in-depth information, I would recommend [Marshall Bruner's YouTube channel](https://www.youtube.com/@MarshallBrunerRF) — his channel is a gem on the subject! The animated graphs on this doc have been created with the [Manim Community](https://docs.manim.community/en/stable/index.html) python library and greatly inspired by Marshall Bruner's code.

### Chirp Signal
FMCW radars involves the continuous transmission and reception of a frequency modulated signal also known as chirp. We will call the transmitted signal **$$T_x$$**{: .text-blue-000} and we are using a sawtooth modulation. The sawtooth modulation linearly sweeps a bandwidth $$B$$ over the chirp period $$T_c$$.

The transmitted signal is reflected from a static object and received by the radar. The received signal is called **$$R_x$$**{: .text-purple-000}. Below is a representation of $$T_x$$ and $$R_x$$'s amplitude and frequency over time.

<img src="{{site.baseurl}}/assets/images/TxRxGraphs.gif" alt="drawing" width="800"/>
{: .text-center}

$$T_x$$ and $$R_x$$'s Amplitude and Frequency as a Function of Time
{: .text-center}

As we see, $$R_x$$ is exactly the same signal as $$T_x$$ but delayed by $$t_0$$, the time of flight (TOF). We can intuitively establish a relationship between $$t_0$$ and the distance to the object $$d$$ :

<div id="eq1" class="fs-5 text-center">
  $$\boxed{t_0 = \frac{2d}{c}} \tag{1}$$
</div>

where $$c = 343 \, \text{m} \cdot \text{s}^{-1}$$ the speed of sound in air for $$T = 293 \, K$$

### Distance
Where pulse radars measure time and TOF to evaluate distance, FMCW radars measure frequencies! The TOF $$t_0$$ cannot be estimated directly since we are sending waves continuously but we have another valuable information : The beat frequency $$f_b$$.

The beat frequency is defined as the frequency equal to the difference between two sinusoids. $$f_b$$ appears in the figure below.

<img src="{{site.baseurl}}/assets/images/fbeat.png" alt="drawing" width="800"/>
{: .text-center}

The Beat Frequency $$f_b$$ appears due to the slight delay between $$T_x$$ and $$R_x$$
{: .text-center}

The geometrical approach is the simplest way to understand how to derive the distance from $$f_b$$. Let's define the slope of the chirp $$s$$ being $$s=\frac{B}{T_c}$$. Using the last figure and Eq [(1)](#eq1) we now have the following relationship :

<div id="eq2" class="fs-5 text-center">
  $$\boxed{f_b = s t_0 = s \frac{2d}{c}} \tag{2}$$
</div>

Thus, the distance can easily be derived :

<div id="eq3" class="fs-5 text-center">
  $$\boxed{d = \frac{c f_b}{2s}} \tag{3}$$
</div>

We know how to compute the distance, that's great! But how do we extract $$f_b$$ ...

### Beat frequency
To extract the beat frequency we need to mix the $$T_x$$ and $$R_x$$. Mixing two signals is essentially multiplying them but it enables us to subtract in the frequency domain. Let's explain 

If we freeze $$T_x$$ and $$R_x$$ at a given time, they're basically two pure sinusisuoids with different frequencies. Let's define $$f_T$$ and $$f_R$$ the instantaneous frequencies of our two signals and $$y_{mix}$$ the mixed signal (the phase of the signals is not relevant here).

At a given time, $$y_{mix}$$ is defined as :

<div id="eq4" class="fs-5 text-center">
  $$ 
  y_{mix} = T_x \cdot R_x = \sin(2 \pi f_{T} t) \cdot \sin(2 \pi f_{R} t) \\
  = \frac{1}{2}\big[\cos\big(2 \pi \textcolor{chartreuse}{\underbrace{\color{white}(f_T-f_R)}_{f_b}} t\big) + \cos\big(2 \pi \textcolor{red}{\underbrace{\color{white}(f_{T}+f_{R})}_{\text{HF component}}} t\big)\big] \tag{4}
  $$
</div>

The mixing operation produces two sinusoids :
- One at the difference of the frequencies $$f_{T}-f_{R}$$
- One at the sum of the frequencies $$f_{T}+f_{R}$$

The high frequency component HF at $$f_{T}+f_{R}$$ can easily be removed with a low pass filter. The remaining signal is a simple sinudoid at the beat frequency $$f_b=f_{T}-f_{R}$$. Below is a representation of the mixing signal operation.


<img src="{{site.baseurl}}/assets/images/Mixing.gif" alt="drawing" width="800"/>
{: .text-center}

$$y_{mix}$$ is the product of $$T_x$$ and $$R_x$$
{: .text-center}



## Radar Design

## Code Implementation
### Send DAC and ADC data
### Receiving the data
## Distance computation