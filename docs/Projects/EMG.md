---
title: EMG
layout: default
parent: Projects
math: mathjax
nav_order: 4
---

# EMG
{: .fs-8 .fw-500 .no_toc}
---

Cool Description
{: .fw-500}

- TOC
{:toc}

## Introduction

SensEdu is equipped with x2 [AD8222] Instrumentation Amplifier ([datasheet]).

Each Amplifier is dual-channel, so we have 4 channels in total. All channels are accessible from the following Jumpers:
* J9: 1st Channel of U5 -> outputs MIC5 - A0 - ADC12_INP4
* J11: 2nd Channel of U5 -> outputs MIC6 - A2 - ADC12_INP9
* J12: 1st Channel of U6 -> outputs MIC7 - A11 - ADC12_INP0
* J20: 2nd Channel of U6 -> outputs MIC8 - A7 - ADC1_INP16

Refer to the [ADC mapping table](/SensEdu/Library/ADC/#adc_mapping) for better understanding.

Circuit for each of the amplifier:

<img src="{{site.baseurl}}/assets/images/amp_circuit.png" alt="drawing"/>
{: .text-center}

Electrode mapping:
* Tip: Red
* Ring: Blue
* Sleeve: Black

## Test

WASD Dark Souls for 4 channels?

## capacitive input
ignore some of the first samples for ADC stabilization

https://devzone.nordicsemi.com/f/nordic-q-a/80796/adc---first-read-is-always-wrong/336435


## Good Resources:
* https://www.nature.com/articles/s41597-022-01484-2
* 

[AD8222]: https://www.analog.com/en/products/ad8222.html?doc=ad8222-KGD.pdf
[datasheet]: https://www.analog.com/media/en/technical-documentation/data-sheets/AD8222.pdf