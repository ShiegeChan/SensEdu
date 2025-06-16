%% data_plotting.m
% Script to visualize saved data
clear;
close all;
clc;

%% Settings
FOLDERNAME = "measurements";
SETNAME = "MusclesSet3";

%% logic
file_path = get_file_path(FOLDERNAME, SETNAME, 1);
if isfolder(file_path)
    return;
end
load(file_path);
current_buffer = buffer;
current_rect = rectified_buffer;
current_env = enveloped_buffer;

for i = 1:1e6
    file_path = get_file_path(FOLDERNAME, SETNAME, i);
    if isfolder(file_path)
        return;
    end
    load(file_path);
    plot_data(processed_x, buffer, buffer_plotting_offsets, rectified_buffer, enveloped_buffer);
    pause(1e-3);
end

%% Functions
function file_path = get_file_path(foldername, setname, index)
    search_pattern = sprintf("%s\\%s\\%d_*", foldername, setname, index);
    file = dir(search_pattern);
    file_path = sprintf("%s\\%s\\%s", foldername, setname, file.name);
end

function plot_data(processed_x, buffer, buffer_plotting_offsets, rectified_buffer, enveloped_buffer)
    for i = 1:size(buffer,1)
        subplot(1,size(buffer,1),i);
        plot(buffer(i,:) - buffer_plotting_offsets(i));
        hold on;
        plot(processed_x, rectified_buffer(i,:));
        plot(processed_x, enveloped_buffer(i,:), 'r', 'linewidth', 2.5);
        ylim([-600,800]);
        %legend(["Raw Data (centered)", "Filtered and Rectified", "Envelope"]);
        hold off;
    end
end