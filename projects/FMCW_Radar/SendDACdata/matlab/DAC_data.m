%% Real-Time DAC Data Plotting
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM8';        % Update according to your setup
ARDUINO_BAUDRATE = 115200;    % Baudrate for serial communication
ACTIVATE_PLOTS = true;        % Enable or disable plotting
ITERATIONS = 1000;             % Number of iterations (or set to "inf" for continuous plotting)

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % Initialize the serial connection

% Initialize a real-time figure
if ACTIVATE_PLOTS
    figure('Name', 'Real-Time DAC Data');
    real_time_plot = plot(nan(1, 1), 'r'); % Initialize empty plot
    ylim([0, 4096]); % 12-bit DAC range
    grid on;
    xlabel("Sample #");
    ylabel("DAC Data (12-bit Value)");
    title("Real-Time DAC Signal");
end

%% Real-Time Data Retrieval Loop
disp("Starting real-time plotting...");
for iteration = 1:ITERATIONS
    % Trigger Arduino to send DAC data
    write(arduino, 't', "char");

    % Read total byte length (sent as a 4-byte uint32)
    total_byte_length = read_total_length(arduino);
    data_length = total_byte_length / 2; % Convert byte length to samples (16-bit)

    % Read the entire DAC data
    dac_data = read_data(arduino, data_length);

    % Update plot in real-time
    if ACTIVATE_PLOTS
        set(real_time_plot, 'YData', dac_data);  % Update Y-axis data
        set(real_time_plot, 'XData', 1:data_length); % Update X-axis data
        drawnow limitrate; % Redraw the plot efficiently
    end

    % Optional: Pause for visualization (depends on chirp duration)
    pause(0.01); % Adjust to match your signal duration
end

%% Set COM Port Back Free
clear arduino;
disp("Finished real-time plotting");

%% Supporting Functions
function total_byte_length = read_total_length(arduino)
    % Reads the 4-byte total length (in bytes) of incoming data from Arduino
    len_bytes = read(arduino, 4, 'uint8'); % Read header
    total_byte_length = typecast(uint8(len_bytes), 'uint32'); % Convert to uint32
end

function data = read_data(arduino, data_length)
    % Retrieve `data_length` samples from Arduino
    total_byte_length = data_length * 2; % Convert samples to bytes
    serial_rx_data = zeros(1, total_byte_length, 'uint8'); % Preallocate space

    % Read data from Arduino
    for i = 1:total_byte_length
        serial_rx_data(i) = read(arduino, 1, 'uint8');
    end

    % Convert byte data to uint16 samples
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end
