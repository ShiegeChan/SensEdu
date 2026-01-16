"""
Basic_UltraSound_4CH_ReadData.py

Reads 4-channel ADC microphone measurements from Arduino via serial
"""

import os
import time
from datetime import datetime

import numpy as np
import matplotlib.pyplot as plt
import serial

# =======================
# Settings
# =======================
ARDUINO_PORT = "COM22"  # replace "COM17" with your serial port
ARDUINO_BAUDRATE = 115200
ITERATIONS = 1000

ACTIVATE_PLOTS = True

DATA_LENGTH = 2048 * 2  # samples per 2 microphones (must match firmware)
CHUNK_SIZE = 32  # bytes per serial read
BYTES_PER_SAMPLE = 2

# =======================
# Serial Setup: select port and baudrate
# =======================
arduino = serial.Serial(
    port=ARDUINO_PORT,
    baudrate=ARDUINO_BAUDRATE,
    timeout=1.0
)

# =======================
# Functions
# =======================
def read_2mic_data(serial_port, data_length, chunk_size):
    """
    Read interleaved data for 2 microphones and split them.
    """
    total_bytes = data_length * BYTES_PER_SAMPLE
    rx_buffer = bytearray(total_bytes)

    bytes_read = 0
    while bytes_read < total_bytes:
        transfer_size = min(chunk_size, total_bytes - bytes_read)
        chunk = serial_port.read(transfer_size)

        if not chunk:
            continue

        rx_buffer[bytes_read:bytes_read + len(chunk)] = chunk
        bytes_read += len(chunk)

    data = np.frombuffer(rx_buffer, dtype=np.uint16)

    mic1 = data[0::2].astype(np.float64)      # interleaving logic to retrieve data from two channels
    mic2 = data[1::2].astype(np.float64)

    return mic1, mic2

def plot_data(mic1, mic2, mic3, mic4):
    """
    Plot 4 microphone channels.
    """
    plt.clf()

    plt.subplot(2, 2, 1)
    plt.plot(mic1)
    plt.ylim(0, 65535)
    plt.title("Microphone 1 data")
    plt.xlabel("Sample #")
    plt.ylabel("ADC1 CH1 16bit")
    plt.grid(True)

    plt.subplot(2, 2, 2)
    plt.plot(mic2)
    plt.ylim(0, 65535)
    plt.title("Microphone 2 data")
    plt.xlabel("Sample #")
    plt.ylabel("ADC1 CH2 16bit")
    plt.grid(True)

    plt.subplot(2, 2, 3)
    plt.plot(mic3)
    plt.ylim(0, 65535)
    plt.title("Microphone 3 data")
    plt.xlabel("Sample #")
    plt.ylabel("ADC2 CH1 16bit")
    plt.grid(True)

    plt.subplot(2, 2, 4)
    plt.plot(mic4)
    plt.ylim(0, 65535)
    plt.title("Microphone 4 data")
    plt.xlabel("Sample #")
    plt.ylabel("ADC2 CH2 16bit")
    plt.grid(True)

    plt.tight_layout()
    plt.pause(0.001)

# =======================
# Main Acquisition Loop
# =======================
data_buffer = []
time_axis = []

start_time = time.perf_counter()

for iteration in range(ITERATIONS):
    # Trigger Arduino
    arduino.write(b"t")

    # Timestamp
    time_axis.append(time.perf_counter() - start_time)

    # Read 4 channels 
    mic1, mic2 = read_2mic_data(arduino, DATA_LENGTH, CHUNK_SIZE)
    mic3, mic4 = read_2mic_data(arduino, DATA_LENGTH, CHUNK_SIZE)

    data_buffer.append([mic1, mic2, mic3, mic4])

    if ACTIVATE_PLOTS:
        plot_data(mic1, mic2, mic3, mic4)

    print(f"Iteration {iteration + 1}/{ITERATIONS}")

# Close serial
arduino.close()

# =======================
# Save measurements into a single file Measurements in uncompressed .npz format
# =======================
os.makedirs("Measurements", exist_ok=True)

timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
file_name = f"Measurements/measurements_{timestamp}.npz"

np.savez(
    file_name,
    data=np.array(data_buffer, dtype=object),
    time_axis=np.array(time_axis)
)

# =======================
# Timing Statistics
# =======================
time_diffs = np.diff(time_axis)
avg_time = np.mean(np.abs(time_diffs))

print(f"Plots are activated: {ACTIVATE_PLOTS}")
print(f"Average time between measurements: {avg_time:.6f} sec")

if ACTIVATE_PLOTS:
    plt.show()