%% data_analysis.m
clc;
clear;
close all;

%% Settings
filename = 'muscles_set_20-May-2025_16-44-07.mat';

%% logic
load(filename);
data = zeros(1,1024*1000);
j = 1;
it = 1;
for i = 1:length(data)
    data(i) = data_collection(1,j,it);
    if mod(i,1024) == 0
        j = 1;
        it = it + 1;
    else
        j = j + 1;
    end
end

plot(data);