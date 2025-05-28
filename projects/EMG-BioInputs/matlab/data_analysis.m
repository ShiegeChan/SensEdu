%% data_analysis.m
clc;
clear;
close all;

%% Settings
filename = 'muscles_set_relax_swings_relax_long_relax_5short_relax_long.mat';

%% logic
load(filename);

data1 = construct_dataset(raw_data);
data2 = construct_dataset(processed_data);

subplot(1,2,1);
plot(data1);
subplot(1,2,2);
plot(data2);

%% processing
data3 = data2 - (mean(data2));
data3 = abs(data3);
data4 = envelope(data3, 2000, "rms");
figure;
plot(data3);
hold on;
plot(data4);

function data = construct_dataset(dataset)
    data = zeros(1,size(dataset,2)*size(dataset,3));
    j = 1;
    it = 1;
    for i = 1:length(data)
        data(i) = dataset(1,j,it);
        if mod(i,size(dataset,2)) == 0
            j = 1;
            it = it + 1;
        else
            j = j + 1;
        end
    end
end