%% main.m
% reads config data and then ADC mics meassurements from Arduino
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM20';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 80; 
MIC_NUM = 8;
mic_name = {"MIC 1", "MIC 2","MIC 3", "MIC 4", "MIC 8", "MIC 6", "MIC 5", "MIC 7"};
VARIANCE_TEST = false; 

PLOT_DISTANCE = true;
PLOT_DETAILED_DATA = true; % make sure to change this in .ino code as well
PLOT_LIMIT = 1; % in meters
PLOT_FIX_X_AXIS = false; % fix x axis to certain amount of measurements
PLOT_FIX_X_AXIS_NUM = 200; % multiple of 10!
PLOT_TIME_AXIS = false; % replace measurements with time in x axis

%% Arduino Setup + Config
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate
DATA_LENGTH = 32 * 32; 


%% Readings Loop

dist_matrix = zeros(MIC_NUM, ITERATIONS);

data_mic1 = zeros(1, ITERATIONS);
data_mic2 = zeros(1, ITERATIONS);
data_mic3 = zeros(1, ITERATIONS);
data_mic4 = zeros(1, ITERATIONS);
detail_info = zeros(MIC_NUM*3, 1024, ITERATIONS);

x_axis = 1:PLOT_FIX_X_AXIS_NUM;
x_shift = PLOT_FIX_X_AXIS_NUM/10;

time_axis = zeros(1, ITERATIONS);
pause(6);
tic;

for it = 1:ITERATIONS
    % if VARIANCE_TEST == true
    %     if (mod(it,100) == 0)
    %         fprintf("Please change position\n");
    %         pause(180);
    %     end
    % end

    % Data readings
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;
    
    % Detailed data: raw microphone data | scaled and removed self
    % reflection | xcorr result
    
    details_matrix = read_mcu_xcorr_details(arduino, MIC_NUM, DATA_LENGTH, 3);
    detail_info(:,:,it) = details_matrix;
    
    % Reading distance directly from the mcu 
    dist_vector = read_mcu_xcorr(arduino, MIC_NUM);
    % if(it > 1 && any(abs(dist_vector - dist_matrix(it-1)) > 0.2))
    %     check_matrix()
    % 
    % end
    

    for i = 1:MIC_NUM
        dist_matrix(i, it) = dist_vector(i);
    end

    % Data plotting
    % if PLOT_DISTANCE==true
    %     subplot_x_size = 1;
    %     if PLOT_DETAILED_DATA == true
    %         subplot_x_size = 4;   
    %         plot_details(details_matrix, MIC_NUM, 4);
    %     end
    %     x_axis = plot_distance(dist_matrix, x_axis, PLOT_FIX_X_AXIS, x_shift, PLOT_TIME_AXIS, time_axis, PLOT_LIMIT, MIC_NUM, it, subplot_x_size);
    % end

end

% set COM port back free
arduino = [];

% save measurements
file_name = sprintf('%s_%s.mat', "ball", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "dist_matrix", "detail_info", "time_axis");

% calculate average time between measurements
buf = time_axis(2) - time_axis(1);
for i = 2:(length(time_axis) - 1)
    buf = mean([buf, (time_axis(i+1) - time_axis(i))]);
end
fprintf("Plots are activated: %s\n", mat2str(PLOT_DISTANCE)); 
fprintf("average time between measurements: %fsec\n", buf);

%%
% for j = 1:ITERATIONS
%     plot_details(detail_info(:,:,j), MIC_NUM, 4);
% end
x_axis = plot_distance(dist_matrix, x_axis, PLOT_FIX_X_AXIS, x_shift, PLOT_TIME_AXIS, time_axis, PLOT_LIMIT, MIC_NUM, it, 1);
%%
m1_dist = dist_matrix(1,:);
m2_dist = dist_matrix(2,:);
m3_dist = dist_matrix(3,:);
m4_dist = dist_matrix(4,:);
m8_dist = dist_matrix(8,:);
m6_dist = dist_matrix(6,:);
m5_dist = dist_matrix(5,:);
m7_dist = dist_matrix(7,:);

dist_matrix = [m1_dist; m2_dist; m3_dist; m4_dist; m8_dist; m6_dist; m5_dist; m7_dist];

figure
for i = 1:8
    switch i
        case 1
            m = "o";
        case 2
            m = "^";
        case 3
            m = "square";
        case 4
            m = "diamond";
        case 5
            m = "v";
        case 6
            m = "hexagram";
        case 7
            m = "pentagram";
        case 8
            m = ">";
    end

    plot(dist_matrix(i, :), 'LineWidth', 2, 'Marker', m); hold on;

end
ylim([0 1])
%xlim([0 time_axis(end)])
grid on
xlabel("time [s]");
ylabel("distance [m]")
legend(mic_name);
title("Microphone distance measurements")

%%
figure
for i = 1:MIC_NUM
    plot(time_axis, dist_matrix(i, :), 'LineWidth', 2); hold on;
end
ylim([0 1])
xlim([0 time_axis(end)])
grid on
xlabel("time [s]");
ylabel("distance [m]")
legend(mic_name);
title("Microphone distance measurements")

%% functions

function plot_details(details_matrix, mic_num, subplot_x_size)
    details_size = subplot_x_size - 1; % exclude distance
    for i = 1:mic_num
        for j = 1:details_size
            subplot(mic_num, subplot_x_size, i + j + details_size*(i-1));
            plot(details_matrix(j + details_size*(i-1), :))

            if j == 1
                title("Raw Microphone Data")
                ylim([0 65535]);
            elseif j == 2
                title("Remapped Data on MCU")
                ylim([-1 1]);
            elseif j == 3
                title("XCORR results on MCU")
            end
        end
    end
end

function x_axis = plot_distance(dist_matrix, x_axis, is_x_fixed_axis, x_shift, is_x_time_axis, time_axis, y_limit, mic_num, current_iteration, subplot_x_size)

    if (current_iteration > (x_axis(end) - x_shift))
        x_axis = x_axis + x_shift;
        diff = length(time_axis) - x_axis(end);
        if (diff < 0)
            x_axis = x_axis + diff;
        end
    end

    for i = 1:mic_num
        subplot(mic_num,subplot_x_size, 1 + subplot_x_size*(i-1));

        if is_x_fixed_axis
            temp = dist_matrix(i, x_axis);

            x = x_axis;
            if is_x_time_axis
                x = time_axis(x_axis);
                x = x(x~=0);
                temp = temp(temp~=0);
            end

            plot(x, temp);
        else
            temp = dist_matrix(i, :);
            temp = temp(temp~=0);

            x = 1:length(temp);
            if is_x_time_axis
                x = time_axis(1:length(temp));
            end

            plot(x, temp);
        end

        %title(mic_name(1, i));
        ylabel("Distance [m]")
        if is_x_time_axis
            xlabel("Time [sec]")
        else
            xlabel("Measurement [#]")
        end
        ylim([0, y_limit]);
    end
end



