"""
Basic_UltraSound_ReadData.py

Reads config data and then ADC mics measurements from Arduino
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
ITERATIONS = 100

ACTIVATE_PLOTS = True
CHUNK_SIZE = 32  # bytes per serial read
DATA_LENGTH = 5142  # number of uint16 samples
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
def read_data(serial_port, data_length, chunk_size):
    """
    Read a fixed number of bytes from serial and convert to uint16 data.
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

    return np.frombuffer(rx_buffer, dtype=np.uint16).astype(np.float64)

def plot_data(data):
    """
    Plot ADC data.
    """
    plt.clf()
    plt.plot(data)
    plt.ylim(0, 65535)
    plt.xlabel("Sample #")
    plt.ylabel("ADC 16-bit value")
    plt.grid(True)
    plt.pause(0.001)

# =======================
# Main Acquisition Loop
# =======================
data_buffer = []
time_axis = []

start_time = time.perf_counter()

for iteration in range(ITERATIONS):
    # Trigger Arduino measurement
    arduino.write(b"t")

    # Timestamp 
    current_time = time.perf_counter() - start_time
    time_axis.append(current_time)

    t_start = time.perf_counter()
    data = read_data(arduino, DATA_LENGTH, CHUNK_SIZE)
    elapsed = time.perf_counter() - t_start

    data_buffer.append(data)

    if ACTIVATE_PLOTS:
        plot_data(data)

    print(f"Iteration {iteration + 1}/{ITERATIONS} - Read time: {elapsed:.4f}s")

# Close serial port
arduino.close()

# =======================
# Save measurements into a single file Measurements in uncompressed .npz format
# =======================
os.makedirs("Measurements", exist_ok=True)

timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
file_name = f"Measurements/measurements_{timestamp}.npz"           

np.savez(  
    file_name,
    data=np.array(data_buffer),
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