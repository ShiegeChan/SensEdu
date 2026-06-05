%% FSK_RX_MATLAB.m
clear;
close all;
clc;

%% FSK Settings

% Plot Processing Steps (slows down the script)
ENABLE_PLOTS = true;

% Sampling Rates
Fs_tx = 480e3;  % TX SR
Fs = 240e3;     % RX SR (must be a multiple of TX)

% Samples per bit
N_tx = 200;
N = round(N_tx * (Fs/Fs_tx));

% FSK Encoding frequencies
% Must be the multiples of fs/N!
% Ensure these frequncies are exactly the same as in FSK_TX sketch
F0 = 13 * Fs / N; % 31200 Hz
F1 = 15 * Fs / N; % 36000 Hz
F = [F0, F1];

% Framing Correction Offset Step
% (Affects a lot the accuracy vs performance trade-off)
HOP_STEPS = 5;
HOP = N/HOP_STEPS;

% Preamble 0xFF00FF00
% Must match with TX Arduino sketch
PREAMBLE = [1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0];

%% Connection Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 2000000;

BUF_SIZE = 64;
CHUNK_SIZE = 64; % Bytes per USB request

MSG_RECORD_WINDOW_SEC = 3;
ITERATIONS = (Fs * MSG_RECORD_WINDOW_SEC) / (BUF_SIZE / 2);

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

%% Readings Loop
half_buf_size = BUF_SIZE / 2;
data = zeros(half_buf_size, ITERATIONS);

% Trigger the measurement
while (true)
    fprintf("Recording the message... (%d seconds)\n", MSG_RECORD_WINDOW_SEC);
    flush(arduino);
    write(arduino, 't', "char");
    
    for it = 1:ITERATIONS
        data(:, it) = read_data(arduino, half_buf_size, CHUNK_SIZE);
    end
    
    data_reshaped = reshape(data, 1, []);

    fprintf("Decoding the message... ");
    tic;
    msg = decode_fsk_message(data_reshaped, PREAMBLE, F, Fs, HOP, N, ENABLE_PLOTS);
    fprintf("(elapsed time: %f seconds)\n", toc);
    if msg ~= ""
        fprintf("Received message: %s\n", msg);
    else
        fprintf("No message detected.\n");
    end
    fprintf("\n");
end

%% Functions
function data = read_data(arduino, buf_size, chunk_size)
    % 2 bytes per sample
    total_byte_length = buf_size * 2;
    serial_rx_data = zeros(1, total_byte_length, 'uint8');
    bytes_read = 0;
    while bytes_read < total_byte_length 
        transfer_size = min(chunk_size, total_byte_length - bytes_read);
        serial_rx_data(bytes_read + 1 : bytes_read + transfer_size) = read(arduino, transfer_size, 'uint8');
        bytes_read = bytes_read + transfer_size;
    end
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end