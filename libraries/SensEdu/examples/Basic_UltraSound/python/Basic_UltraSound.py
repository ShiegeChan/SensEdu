import serial
import numpy as np
import matplotlib.pyplot as plt
import time

# Open the serial connection
ser = serial.Serial('COM4', 115200, timeout=0.1)  # replace 'COM3' with your serial port

def send_trigger():
    ser.write(b't')

def receive_data():
    data = bytearray()
    while True:
        if ser.in_waiting > 0:
            chunk = ser.read(ser.in_waiting)
            data.extend(chunk)
        else:
            time.sleep(0.01)  # wait for a short period
            if ser.in_waiting == 0:
                break
    return data

def plot_data(data):
    mic_data = np.frombuffer(data, dtype=np.uint16)
    plt.plot(mic_data)
    plt.xlabel('Sample Index')
    plt.ylabel('Amplitude')
    plt.title('Received Data')
    plt.show()

# Send the trigger character
send_trigger()

# Receive the data
data = receive_data()

# Plot the data
plot_data(data)

# Close the serial connection
ser.close()