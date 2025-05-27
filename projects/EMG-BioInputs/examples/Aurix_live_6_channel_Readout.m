%Author
clc;
clear;
close all;

%%
%This part of the code is for wwinitializing the computer inputs
import java.awt.Robot;
import java.awt.event.*;
global robot
robot = java.awt.Robot();
robot.delay(2000);

global key_set
key_set = [false,false,false,false,false,false]; %Keys number 0,1,2,3 are either pressed or not pressed
%Carefull! MATLAB Indices start at 1, always adress key_set(1+muscle_number)



%%
%UDP Port setup
Port = 5005;
NumberOfChannels = 6;
global ValuesPerChannel
ValuesPerChannel = 2000; % Number of values expected per channel, defined by Aurix
ValuesPerRead = ValuesPerChannel * NumberOfChannels; % Number of values expected per read, defined by Aurix
udpObj = udpport("LocalPort", Port);
global SampleFrequency
SampleFrequency = 156250/4;

%%
% Parameters
numValuesToDisplay = 156250;

connected_muscles = 1:6; %All muscle groups
connected_muscles = [1,2,3,4]; %Overwrite with the ones actually connected

%Determine the bounds to decide on relaxed/flexed
percent_margin_1 = 2; %For player 1
percent_margin_2 = 0.5; %For player 2
values_to_check = 50000;

%Tell which Channels are assigned to which player; Enter 1 or 2 in the six
%positions
channel_assigning = [2,1,1,2,2,2];

%Filter parameters
global filter_state
filter_state = zeros (6,2); %Each line is for one channel

%% Preallocate Buffer Sizes
Data = zeros(ValuesPerRead, 1);
channel_buffers = zeros(numValuesToDisplay,NumberOfChannels,'int16');
filtered_buffers = channel_buffers;


%%
% Create figure and axes
fig = figure;
y_low = -2000; %Default -32768
y_high = 22000; %Default 32767
x_low = 0;
x_high = numValuesToDisplay;
% Channel 0
subplot(6, 1, 1);
plot0 = plot(channel_buffers(:,1+0));
title('Channel 0');
xlabel('Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits
xlim([x_low ,x_high]);

% Channel 1
subplot(6, 1, 2);
plot1 = plot(channel_buffers(:,1+1));
title('Channel 1');
xlabel( 'Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits
xlim([x_low ,x_high]);

% Channel 2
subplot(6, 1, 3);
plot2 = plot(channel_buffers(:,1+2));
title('Channel 2');
xlabel('Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits
xlim([x_low ,x_high]);

% Channel 3
subplot(6, 1, 4);
plot3 = plot(channel_buffers(:,1+3));
title('Channel 3');
xlabel('Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits
xlim([x_low ,x_high]);

% Channel 4
subplot(6, 1, 5);
plot4 = plot(channel_buffers(:,1+4));
title('Channel 4');
xlabel( 'Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits
xlim([x_low ,x_high]);

% Channel 5
subplot(6, 1, 6);
plot5 = plot(channel_buffers(:,1+5));
title('Channel 5');
xlabel('Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits
xlim([x_low ,x_high]);

plot_array = [plot0,plot1,plot2,plot3,plot4,plot5];
x_data = 1:1:numValuesToDisplay; %The x-axis over which the graphs will be plotted



%%
% Main loop to continuously receive and update data
i=0;
while true
     
    % Read from UDP
    Data = read(udpObj, ValuesPerRead, "int16");
    Data = Data';
    
    %Split up the long Data-block into the respective channels; Defined by
    %Aurix
   

    for n = 1:6
        channel_buffers(:,n)=[channel_buffers((ValuesPerChannel + 1:end),n) ; Data((n-1)*ValuesPerChannel+1:n*ValuesPerChannel)];
        %filtered_buffers(:,n)=[filtered_buffers((ValuesPerChannel + 1:end),n) ; Data((n-1)*ValuesPerChannel+1:n*ValuesPerChannel)];
        %filtered_buffers(:,n) = Filter_Buffer(filtered_buffers(:,n),n-1);
    end

    
    

    % Update plot data for each channel
    for n = 1:6
    set(plot_array(n), 'YData', channel_buffers(:,n),'XData', x_data);
    end

    %FOR TEST
    %set(plot_array(6), 'YData', filtered_buffers(:,3),'XData', x_data);
    
    drawnow limitrate;  % Update plot only every 50 milliseconds to save computing performance



    %Optional for pausing and recording data
    i=i+1;
    %fprintf("%d \n",i);
    if (i==100)
        for n = 1:6
        fprintf("Max channel %d: %d \n", n, max(channel_buffers(:,n)));
        fprintf("Min channel %d: %d \n", n, min(channel_buffers(:,n)));
        end
         
    end
end



%%
function [OutBuffer] = Filter_Buffer(InBuffer,muscle_number)
%Notch filter 50Hz the last measured values in the InBuffer and put the result in OutBuffer
%Johannes Kohlhaas


% Fs = 156250/4;   % Sampling frequency. Defined by the Aurix
% Fnyq = Fs/2; % Nyquist frequency is half the sampling frequency
% 
% F0 = 50;   % Interference is at 50 Hz
% BW = 2;    % Choose a bandwidth factor of 2 Hz
% 
% [num1,den1] = designNotchPeakIIR(Response="notch",CenterFrequency=F0/Fnyq,Bandwidth=BW/Fnyq,FilterOrder=2);

global ValuesPerChannel
global filter_state

num1 = [0.999839176323180,-1.99961368195782,0.999839176323148]; %For 10 MHz Aurix frequency
den1 = [1,-1.99961368195782,0.999678352646328];                 %For 10 MHz Aurix frequency


AC_Buffer = InBuffer - mean (InBuffer(end-10000:end)); %Remove the DC Part, which is necessary for the Notch filter
OutBuffer = AC_Buffer;
[OutBuffer(end-ValuesPerChannel:end),filter_state(muscle_number+1,[1,2])] = filter(num1,den1,AC_Buffer(end-ValuesPerChannel:end),filter_state(muscle_number+1,[1,2]));
OutBuffer = OutBuffer + mean (InBuffer(end-10000:end));





end

