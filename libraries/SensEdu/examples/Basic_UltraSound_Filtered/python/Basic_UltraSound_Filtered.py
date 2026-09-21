"""
Basic_UltraSound_Filtered.py

Triggers an ultrasonic measurement with 't', reads the raw and the filtered
microphone buffer from the Arduino, plots both on one figure and saves all
iterations into Measurements/.

The raw buffer is sent unscaled to halve its transfer size, so this script has
to apply exactly the same rescaling the firmware uses before filtering.
Any other scaling would make the comparison between the two traces unfair.

DATA_LENGTH must match the firmware.
"""

import os
from datetime import datetime

import numpy as np
import matplotlib.pyplot as plt
import serial

# =======================
# Settings
# =======================
ARDUINO_PORT = "COM22"  # Replace with your serial port
ARDUINO_BAUDRATE = 115200
ITERATIONS = 100

CHUNK_SIZE = 32  # Bytes per serial read
DATA_LENGTH = 2048  # Number of samples per buffer (must match the firmware)

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
def read_buffer(serial_port, data_length, bytes_per_sample, dtype, chunk_size):
    """
    Read a fixed number of bytes from serial and convert to the given dtype.
    """
    total_bytes = data_length * bytes_per_sample
    rx_buffer = bytearray(total_bytes)

    bytes_read = 0
    while bytes_read < total_bytes:
        transfer_size = min(chunk_size, total_bytes - bytes_read)
        chunk = serial_port.read(transfer_size)

        if not chunk:
            continue

        rx_buffer[bytes_read:bytes_read + len(chunk)] = chunk
        bytes_read += len(chunk)

    return np.frombuffer(rx_buffer, dtype=dtype).astype(np.float64)

def read_data(serial_port, data_length, chunk_size):
    """
    Read the raw buffer followed by the filtered one. The raw counts must be
    rescaled exactly as the firmware does, otherwise the traces do not match.
    """
    raw = read_buffer(serial_port, data_length, 2, np.uint16, chunk_size)
    filtered = read_buffer(serial_port, data_length, 4, np.float32, chunk_size)

    rescaled = (2.0 * raw) / 65535.0 - 1.0
    return rescaled, filtered

def plot_data(rescaled, filtered):
    """
    Plot the rescaled raw signal and the filtered one on the same axes.
    """
    plt.clf()
    plt.plot(rescaled, label="Raw (rescaled)", linewidth=0.8, alpha=0.5)
    plt.plot(filtered, label="Filtered", linewidth=0.8)
    plt.xlabel("Sample #")
    plt.ylabel("Amplitude")
    plt.legend(loc="upper right")
    plt.grid(True)
    plt.pause(0.001)

# =======================
# Main Acquisition Loop
# =======================
rescaled_buffer = []
filtered_buffer = []

for iteration in range(ITERATIONS):
    # Trigger Arduino measurement
    arduino.write(b"t")

    rescaled, filtered = read_data(arduino, DATA_LENGTH, CHUNK_SIZE)

    rescaled_buffer.append(rescaled)
    filtered_buffer.append(filtered)

    plot_data(rescaled, filtered)

    print(f"Iteration {iteration + 1}/{ITERATIONS}")

# Set COM port back free
arduino.close()

# =======================
# Save Measurements
# =======================
# Stored as a single uncompressed .npz file inside Measurements/
os.makedirs("Measurements", exist_ok=True)

timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
file_name = f"Measurements/measurements_{timestamp}.npz"

np.savez(
    file_name,
    rescaled=np.array(rescaled_buffer),
    filtered=np.array(filtered_buffer)
)

plt.show()
