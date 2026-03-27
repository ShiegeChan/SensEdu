%% data_plotting.m
% Script to visualize saved data
clear;
%close all;
clc;

%% Settings
FOLDERNAME = "measurements";
SETNAME = "MuscleSet4";

%% logic
file_path = get_file_path(FOLDERNAME, SETNAME, 1);
if isfolder(file_path)
    return;
end
load(file_path);
current_buffer = buffer;
current_rect = rectified_buffer;
current_env = enveloped_buffer;

tic;
for i = 1:1e6
    file_path = get_file_path(FOLDERNAME, SETNAME, i);
    if isfolder(file_path)
        return;
    end
    load(file_path);
    plot_data(processed_x, buffer, buffer_plotting_offsets, rectified_buffer, enveloped_buffer);
    plot_max(size(buffer, 2), current_max, all_history_max, last_1min_max, last_5sec_max);
    pause(1e-16);
    toc
end

%% Functions
function file_path = get_file_path(foldername, setname, index)
    search_pattern = sprintf("%s\\%s\\%d_*", foldername, setname, index);
    file = dir(search_pattern);
    file_path = sprintf("%s\\%s\\%s", foldername, setname, file.name);
end

function plot_data(processed_x, buffer, buffer_plotting_offsets, rectified_buffer, enveloped_buffer)
    for i = 3 % 1:size(buffer,1)
        %subplot(1,size(buffer,1),i);
        plot(buffer(i,:) - buffer_plotting_offsets(i));
        hold on;
        plot(processed_x, rectified_buffer(i,:));
        plot(processed_x, enveloped_buffer(i,:), 'r', 'linewidth', 2.5);
        ylim([-600,800]);
        %legend(["Raw Data (centered)", "Filtered and Rectified", "Envelope"]);
        hold off;
    end
end

function plot_max(max_index, current_max, all_history_max, last_1min_max, last_5sec_max)
    for i = 3 % 1:size(current_max, 1)
        %subplot(1,size(current_max, 1),i);
        hold on;
        plot([1,max_index], [current_max(i), current_max(i)], 'color', '#29505d', 'linewidth', 2.5);
        plot([1,max_index], [all_history_max(i), all_history_max(i)], 'color', '#010f1c', 'linewidth', 2.5);
        plot([1,max_index], [last_1min_max(i), last_1min_max(i)], 'color', '#304529', 'linewidth', 2.5);
        plot([1,max_index], [last_5sec_max(i), last_5sec_max(i)], 'color', '#4a6741', 'linewidth', 2.5);
        hold off;
    end
end