---
title: Ultrasonic Ranging
layout: default
math: mathjax
parent: Projects
nav_order: 1
---


# Ultrasonic Ranging
{: .fs-8 .fw-500}
---

- TOC
{:toc}

## Introduction

Measuring distance is one of the most common measurements that have to be acquired in many applications across different fields. It referes to measuring distance, lenght or range of some object. The simplest way of measuring distance is using rulers. The more sophisticated methods are transit-time methods and interfereometer methods. 

Ranging is technique that measures distance or slant range from the observer to a target, especially a far and moving target. Active methods use unilateral transmission and passive reflection. Active rangefinding methods include laser (lidar), radar, sonar, and ultrasonic rangefinding. Ultrasonic ranging is mostly used in medical field, but it is becoming more popular in other fields and applications where, for example, solutions using radio waves are difficult to deploy due to its cost of production as well as the high computational demand. So in the cases where the focus is on cost reduction and design simplification, as well as when the limited operational range is not a concern, ultrasonic ranging can be a perfect solution. 

Using the SensEdu ultrasonic transducer, we demonstrate ranging technique using sound waves to obtain distance measurements between the transducer and an arbitrary object.  

The following sections will provide a step-by-step guide through the implementation of object ranging using ultrasound in three-dimensional space. 


## Theoretical concept 
An ultrasonic transciever transmits a short sound pulse with every high pulse power. The pulse is directed by the directivity of the speaker and propagates in the given direction with the speed of sound. 
If there is an obstacle (an object) in this direction, a part of the transmitted energy is scattered in all directions. A very small portion of the wave is also reflected back to the transciever. This information is received by the transducer and further evaluated to calculate the distance. Since the propagation of sound waves is constant, the distance is determined from the runtime of the high-frequency transmitted signal. The actual range of a target from the transducer is known as a slant range - a line of sight distance between the transciever and the object illuminated. The same working principle holds for radar systems which can be seen in the following animation. 

<img src="{{site.baseurl}}/assets/images/working_principle.gif" alt="drawing" width="507"/> 


The distance calculation is based on calculation of time difference between the sent pulse and the received pulse by the transciever. Therefore, the following is the formula for the slant range: 

$$
\begin{equation}
d = \frac{c\cdot t}{2}
\label{eq:slant range}
\end{equation}
$$

where *t* is the measured runtime in seconds, *c* is the speed of sound in air (343 m/s) and d is the slant range in meters. 

## Hardware Setup 

The following section describes the hardware used to obtain the distance measurements. First, the system architecture is shown. 

We use SensEdu transducer board equipped with two ultrasonic speakers and four MEMS microphones with resonant frequencies in ultrasonic range. The microphones are placed in a square shape configuration. The board is connected to Arduino GIGA R1 equipped with STM32 microcontroller.

<img src="{{site.baseurl}}/assets/images/sensedu_dm.png" alt="drawing" width="507"/> 

## Signal Processing

When the audio signal is acquired and it goes through ADC module, a digital representation of the sound wave is the start of the signal processing chain. We call this signal the raw signal. To get the information about the distance, we use the cross-correlation technique. It compares the received signal with the sent signal and the time where the similarity between the two signals is the highest, corresponds to the distance of the detected object. 

The following data acquisition flow chart explains the algorithm. 
<img src="{{site.baseurl}}/assets/images/distance-measurement-block-diagram.png" alt="drawing" width="507"/> 

To generate the measurements, first burst sine signal with 32kHz frequency is sent through DAC to the speaker. The sine wave is stored in a lookup table (LUT). 

After the burst is sent, ADC starts to collect the data microphones are receiving. The raw data is represented with 16bit values corresponding to the changes in pressure received by microphones. 

### Rescaling the ADC signal
The ADC signal has values in range [0, 65535]. Therefore, we first rescale the signal to the range [-1, 1] with each sample being a 4-byte float value. 

{: .NOTE}
Because of the way multi-channel ADC signals are stored in memory using SensEdu library, the channel number has to be passed to the function we are using. 

### Filtering the rescaled signal

The microphones have a large bandwidth of 100kHz and the signal is easily distorted by the audible sound. That is why the filtering around the central frequency of 32kHz is needed. Therefore, a Finite Impulse Response (FIR) bandpass filter is applied to the previously resacaled ADC signal. The filter also removes the unwanted DC component, as well as high-frequency noise that is also naturally present. 

### Cross-correlation
Finally, the cross-correlation method is performed on the filtered signal and output signal from the speaker. The result of the cross-correlation is number of lag samples between the two signals. Then, the distance is computed as: 

$$
\begin{equation}
d = \frac{(lag\_samples \cdot sample\_time)}{sampling\_rate \cdot 2}
\end{equation}
$$ 

There are two modes available for sending the data to the PC by setting a value of a macro. Setting the macro value to *true* sends the complete raw-, rescaled-, and filtered data with distance measurements to the PC. Otherwise, only the distance measurements will be sent to the PC.  

{: .NOTE}
For faster measurements, cross-correlation is performed directly on the microcontroller. However, if the time-efficiency is not
a requirement, setting a macro *"LOCAL_XCORR"* to *false* will directly send raw ADC data to the PC. 


## Receiving the Data

The communication is done using a Half Duplex method and UART communication protocol - each sender and receiver can transmit the data but not at the same time. This is needed since the data acqusition is started only after the PC (sender) sends the starting signal to the board. After the starting signal, data acquisition and data sending is performed for a predefined amount of time. 

Since we are using serial communication, the data is received one bit at a time. To start receiving the data from the microcontroller, the PC initiates the communication by sending the starting byte. 

<img src="{{site.baseurl}}/assets/images/serial_communication_registers.png" alt="Serial communication between the mcu and the pc"/>
{: .text-center}

This diagram depicts one iteration of PC receiving the data. This continues for number of iterations that are specified in the MATLAB script. Then, the data is plotted for visualization.

{: .IMPORTANT}
Make sure that *PLOT_DETAILED_DATA* in MATLAB matches the value of *XCORR_DETAILS*, as well as to set *DATA_LENGTH* in MATLAB to be the same as *STORE_BUF_SIZE* macro. Make sure to choose the correct *ARDUINO_PORT* to match the real one. 

## Showcase 

This section shows an examople of determining the object distance using the described protocol above. When there is no object in the defined field of view (FOV) of the measurement system, the raw signal of the microphone looks the following way:

<img src="{{site.baseurl}}/assets/images/basic_ultrasound_no_object.gif" alt="drawing" width="507"/> 

The peak showing in the first samples of the signal represents the audio-coupling effect coming from the speaker that transmits the inital pulse. This determines the minimum anambiguous distance that can be measured by the object and it corresponds to about 20cm from the measurement system. 

If there is an object in the defined FOV, the signal shows two peaks, the second representing the object distance:

<img src="{{site.baseurl}}/assets/images/basic_ultrasound_object.gif" alt="drawing" width="507"/> 

By moving the object in space, the peak will move in time accordingly. Using the above formula, considering the speed of sound in the air constant, the distance value can be determined. 

The following figure represents the measurement acquisition in time, portraying the intermediate processing steps as well. The first column represents the distance in time, while the last three columns represent the current measurement being taken. Each row represents measurements for each microphone on the board. 

<img src="{{site.baseurl}}/assets/images/example-distance-ranging.png" alt="drawing" width="607"/> 


## Considerations and improvements
The quality of the measurements received is highly dependent on many different aspects, one of the most influential being the characteristic of the object itself. The precision of the object distance depends on its size and material, as well as speed of the object in case it is moving. For example, larger object and object made of material with high reflectivity will result in better measurements than the small object with lower reflectivity. This fact also determines the maximum phyiscal range at which an object can be measured, and it is not absolute. The reflectivity of the object is very important since it directly influences the strength of the reflected signal. When the signal reflection is weak, the measurement more difficult to obtain. 

Another thing to consider is that the algorithm is suitable for distance measurement of a single object. In case there are multiple object in FOV, there is no way to distinguish between them. A good improvement could be resolving this issue and simultaniously recieve the distance of multiple objects. 


## Reference
[Radar Working Principle](https://www.radartutorial.eu/01.basics/Distance-determination.en.html)