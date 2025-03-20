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
This chapter explains the fundamentals of FMCW radars in order to better understand their magic. For more in-depth information, I would recommend [Marshall Bruner's YouTube channel](https://www.youtube.com/@MarshallBrunerRF) — it's a gem on the subject!

### Chirp Signal
FMCW radars involves the continuous transmission and reception of a frequency modulated signal also known as chirp. We will call the transmitted signal **Tx**{: .text-purple-000} and we are using a sawtooth modulation. The sawtooth modulation linearly sweeps a bandwidth $$B$$ over the chirp period $$T_c$$.

The transmitted signal is reflected from a static object and received by the radar. The received signal is called **Rx**{: .text-purple-000}. Below is a representation of Tx and Rx's frequency over time

<!-- <img src="{{site.baseurl}}/assets/images/Tx.png" alt="drawing" width="300"/>
{: .text-center} -->

Tx and Rx's frequency as a function of time
{: .text-center}

As we see, Rx is exactly the same signal as Tx but delayed by $$t_0$$, the time of flight (TOF). We can intuitively establish a relationship between $$t_0$$ and the distance to the object $$d$$ :

<div id="eq1" class="fs-5 text-center">
  $$t_0 = \frac{2d}{c} \tag{1}$$
</div>

where $$c = 343 \, \text{m} \cdot \text{s}^{-1}$$ the speed of sound in air for $$T = 293 \, K$$

### Distance
Where pulse radars measure time and TOF to evaluate distance, FMCW radars measure frequencies. Unfortunately, the TOF $$t_0$$ cannot be estimated directly since we are sending waves continuously but we have another valuable information : The beat frequency $$f_b$$.

The beat frequency is defined as the frequency equal to the difference between two sinusoids. $$f_b$$ appears in the figure below.

<video muted controls playsinline>
    <source src="{{site.baseurl}}/assets/videos/TxRx.webm" type="video/webm">
    Video is broken.
</video>

The beat frequency $$f_b$$ appears due to the slight delay between Tx and Rx
{: .text-center}

The geometrical approach is the simplest way to understand how to derive the distance from $$f_b$$. Let's define the slope of the chirp $$s$$ being $$s=\frac{B}{T_c}$$. Using the last figure and Eq [(1)](#eq1) we now have the following relationship :

<div id="eq2" class="fs-5 text-center">
  $$f_b = s t_0 = s \frac{2d}{c} \tag{2}$$
</div>

Thus, the distance can easily be derived :

<div id="eq3" class="fs-5 text-center">
  $$d = \frac{c f_b}{2s} \tag{3}$$
</div>

We know how to compute the distance, that's great! But how do we extract $$f_b$$ ...

### Beat frequency


## Radar Design

## Code Implementation
### Send DAC and ADC data
### Receiving the data
## Distance computation