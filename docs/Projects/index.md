---
title: Projects
layout: default
nav_order: 5
permalink: /projects/
---


# Project: Ultrasonic Distance Measurements

Using the latest SensEdu Shield on an Arduino Giga R2 platform, we want to get the distance measurements for all 4 MEMS microphones. 
In this project, we use SensEdu Library and DMA approach for fast and efficient data acquisition. This can represent a prerequisitie 
for other projects for further processing the distance measurements. The data from the microphones is processed and sent via Serial Communication on a PC. 

The project is divided into 2 main parts: 
* Arduino and SensEdu: Data acquisition and sending the measurements data
* PC and MATLAB: Starting the data acquisition and receiving the measurements data

The following diagram depicts the main project concept. 

// TODO: put the diagram here

## Serial Communication 

The communication is done using a Half Duplex method and UART communication protocol - each sender and receiver can transmit the data but not at the same time. This is needed since the data acqusition is started only after the PC (sender) sends the starting signal to the board. After the starting signal, data acquisition and data sending is performed for a predefined amount of time. 

## Data Acquisition and Sending the Data

The following data acquisition flow chart explains the algorithm. 

Since the microcontroller is ecquiped with three ADCs and we use four microphones, we use two ADCs - ADC1 and ADC2 accessing through two channels all four microphones. The ADC sampling rate is set to 250 kHz. 

{: .important }
The exact sampling rate is very important for exact measurement acquisition. Make sure it matches throughout the code. 

### About the Sampling Rate

In this project, the ADC sampling rate is set to 250 kHz. However, the measured sampling rate slightly differs from this value. To measure the real sampling rate, a MATLAB script "measure_sampling_rate.m" is provided. This value is defined as a constant parameter "ACTUAL_SAMPLING_RATE". 




## Receiving the Data











