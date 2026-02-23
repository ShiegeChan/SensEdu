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

The software architecture is divided into two main modules: the **Signal Generation** on the transmitter side and the **Digital Signal Processing (DSP)** on the receiver side. Both modules are optimized to leverage the Arduino GIGA’s hardware capabilities.


### Transmitter

The transmitter’s primary role is to convert digital data into a continuous FSK-modulated acoustic wave, i.e. the current implementation handles waveform synthesis directly, ensuring a more flexible and integrated approach. Im order to do that, the system utilizes the internal 12-bit DAC to produce high-resolution sine waves. By switching between the two target frequencies mentioned above, the transmitter creates the necessary frequency shifts to encode information. To improve stability and frequency precission, a phase accumulation logic is defined for each frequency.

Due to the finite size of the hardware memory buffer, the maximun message length is set to **30 characters**. This limit is carefully calculated considering the **200 samples** allocated per bit; this duration provides the ultrasonic transducer with sufficient time to stabilize and adapt to each frequency shift, ensuring a clean transition between logic states.

To make sure the wave is perfectly continuous, the system uses the SENSEDU_DAC_MODE_BURST_WAVE. By connecting the dma_buffer directly to the DAC through DMA, the transmission stays stable and avoids any gaps or delays between the bits. For more information about DAC configurations, go to [DAC_Burst_Sine](https://sensedu-shield.com/library/dac/#dac_burst_sine).

```c
// Maximum number of characters to send between separate messages
const uint16_t MAX_MESSAGE_LENGTH = 30;

// Arbitrary chosen number to send ~10-12 cycles per bit
const uint16_t SAMPLES_PER_BIT = 200;

// DMA buffer size
const uint16_t MAX_LUT_SIZE = (MAX_MESSAGE_LENGTH + PREAMBLE_LENGTH) * SAMPLES_PER_CHARACTER; 

volatile SENSEDU_DAC_BUFFER(dma_buffer, MAX_LUT_SIZE);
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = DAC_CH1, 
    .sampling_freq = SAMPLE_RATE,
    .mem_address = (uint16_t*)dma_buffer,
    .mem_size = MAX_LUT_SIZE, 
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = 1
};
```
The main core of the transmitter is the ``` construct bit``` function, which translates logical bits into phzsical sound waves. During the process, the function calculates a sine value for every sample and constantly updates the phase to ensure the wave is smooth, avoiding any sudden jumps between bits. Finally, the signal is shifted and scaled to use the full wange of the hardware.

```c
// Fills the buffer with one bit worth of data
void construct_bit(bool bit, float* phase, uint16_t* buf_pos) {
    float phase_inc = bit ? PHASE_INC1 : PHASE_INC0;
    for (size_t i = 0; i < SAMPLES_PER_BIT; i++) {
        *phase += phase_inc;
        if (*phase > TWO_PI) {
            *phase -= TWO_PI;
        }

        float sample = sinf(*phase);
        dma_buffer[*buf_pos] = (uint16_t)((sample + 1.0f) * 2047.5f);
        (*buf_pos)++;
    }
}
```
Once we have the logic to create a single bit, we need to organize the entire message in memory. The ```construct_buffer``` function acts as the "architect" of the transmission by preparing a continuous stream of data. First, the system clears the memory buffer to ensure no old data interferes with the new message. Then, it generates a **Preamble**, which is a repetitive pattern of '1's and '0's. This is crucial because it acts as a "wake-up call" for the receiver, allowing it to detect that a transmission is starting and to synchronize its clock. Finally, the function breaks down each character of the message into its 8 individual bits and calls the ```construct_bit``` function to fill the buffer with the corresponding 31 kHz or 35 kHz waves.

```c

// Fills the buffer with the message encoded via 12-bit values of ASCII characters
void construct_buffer(uint8_t* data, uint8_t num_bytes) {

    // Position in a LUT buffer
    uint16_t position = 0;

    // Current phase of the output sine wave
    float phase = 0.0f;

    // Clear the entire LUT first with DC level
    for (size_t i = 0; i < MAX_LUT_SIZE; i++) {
        dma_buffer[i] = 0x000;
    }

    // Preamble 10101010 x PREAMBLE_LENGTH times
    for (size_t i = 0; i < PREAMBLE_LENGTH * BIT_PER_CHARACTER; i++) {
        construct_bit(i % 2, &phase, &position);
    }

    // Payload
    for (size_t byte_idx = 0; byte_idx < num_bytes; byte_idx++) {
        uint8_t cur_byte = data[byte_idx];
        for (size_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            bool bit = (cur_byte >> (7 - bit_idx)) & 1;
            construct_bit(bit, &phase, &position);
        }
    }
}
```
Finally ```send_message``` function is responsible for the actual execution of the transmission. It first triggers the buffer assembly to prepare the entire message in memory. Once the data is ready, it enables the DAC hardware to begin streaming the acoustic wave. 

```c
// Transmits the entire constructed message
void send_message(uint8_t* data, uint8_t num_bytes) {
    construct_buffer(data, num_bytes);
    SensEdu_DAC_Enable(DAC_CH1);
    while (!SensEdu_DAC_GetBurstCompleteFlag(DAC_CH1));
    SensEdu_DAC_ClearBurstCompleteFlag(DAC_CH1);
    SensEdu_DAC_Disable(DAC_CH1); // Clean shutdown
}
```

The final component of the transmitter is the main loop, which acts as the "control center" of the system. Its primary task is to constantly monitor the Serial Port for any incoming text from the user. Once the message is typed, the loop ensures it does not exceed the ```MAX_MESSAGE_LENGTH```, and also performs a quick clean up by removing any extra line breaks. Finally, the system prints and sends the message through an acoustic wave into the air.

```c
void loop () {
    if (Serial.available() > 0) {
        length = 0;
        while (Serial.available() > 0 && length < MAX_MESSAGE_LENGTH) {
            message[length] = Serial.read();
            length++;
        }
        
        while (length > 0 && (message[length - 1] == '\n' || message[length - 1] == '\r')) {
            length--;
        }
        
        if (length > 0) {
            Serial.println("Transmitted message: ");
            Serial.write(message, length);
            Serial.println("");
            send_message(message, length); 
        }
    }
}
```

### Receiver 
