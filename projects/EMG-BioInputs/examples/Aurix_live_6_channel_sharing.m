clc;
clear;
close all;


%%
%UDP Port setup
Port = 5005;
NumberOfChannels = 6;
ValuesPerChannel = 2000; % Number of values expected per channel, defined by Aurix
ValuesPerRead = ValuesPerChannel * NumberOfChannels; % Number of values expected per read, defined by Aurix
udpObj = udpport("LocalPort", Port);

%%
% Parameters
numValuesToDisplay = 156250;
SampleFrequency = 156250;

%% Preallocate Buffer Sizes
Data = zeros(ValuesPerRead, 1);


channel_buffer_0 = zeros(numValuesToDisplay, 1, 'int16');
channel_buffer_1 = zeros(numValuesToDisplay, 1, 'int16');
channel_buffer_2 = zeros(numValuesToDisplay, 1, 'int16');

channel_buffer_3 = zeros(numValuesToDisplay, 1, 'int16');
channel_buffer_4 = zeros(numValuesToDisplay, 1, 'int16');
channel_buffer_5 = zeros(numValuesToDisplay, 1, 'int16');


%%
% Create figure and axes
fig = figure;
y_low = -2000; %Default -32768
y_high = 15000; %Default 32767
% Channel 0
subplot(6, 1, 1);
plot0 = plot(channel_buffer_0);
title('Channel 0');
xlabel('Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits

% Channel 1
subplot(6, 1, 2);
plot1 = plot(channel_buffer_1);
title('Channel 1');
xlabel( 'Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits

% Channel 2
subplot(6, 1, 3);
plot2 = plot(channel_buffer_2);
title('Channel 2');
xlabel('Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits

% Channel 3
subplot(6, 1, 4);
plot3 = plot(channel_buffer_3);
title('Channel 3');
xlabel('Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits

% Channel 4
subplot(6, 1, 5);
plot4 = plot(channel_buffer_4);
title('Channel 4');
xlabel( 'Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits

% Channel 5
subplot(6, 1, 6);
plot5 = plot(channel_buffer_5);
title('Channel 5');
xlabel('Sample Number');
ylabel('Value');
ylim([y_low ,y_high]);  % Set y-axis limits



%%
% Main loop to continuously receive and update data
while true
     
    % Read from UDP
    Data = read(udpObj, ValuesPerRead, "int16");
    Data = Data';
    
    %Split up the long Data-block into the respective channels; Defined by
    %Aurix
    channel_buffer_0 = [channel_buffer_0(ValuesPerChannel + 1:end); Data(1:ValuesPerChannel)];
    channel_buffer_1 = [channel_buffer_1(ValuesPerChannel + 1:end); Data(ValuesPerChannel+1:ValuesPerChannel*2)];
    channel_buffer_2 = [channel_buffer_2(ValuesPerChannel + 1:end); Data(ValuesPerChannel*2+1:ValuesPerChannel*3)];
    channel_buffer_3 = [channel_buffer_3(ValuesPerChannel + 1:end); Data(ValuesPerChannel*3+1:ValuesPerChannel*4)];
    channel_buffer_4 = [channel_buffer_4(ValuesPerChannel + 1:end); Data(ValuesPerChannel*4+1:ValuesPerChannel*5)];
    channel_buffer_5 = [channel_buffer_5(ValuesPerChannel + 1:end); Data(ValuesPerChannel*5+1:ValuesPerChannel*6)];
   
    % Update plot data for each channel
    set(plot0, 'YData', channel_buffer_0,'XData', 1:numel(channel_buffer_0));
    set(plot1, 'YData', channel_buffer_1,'XData', 1:numel(channel_buffer_1));
    set(plot2, 'YData', channel_buffer_2,'XData', 1:numel(channel_buffer_2));
    set(plot3, 'YData', channel_buffer_3,'XData', 1:numel(channel_buffer_3));
    set(plot4, 'YData', channel_buffer_4,'XData', 1:numel(channel_buffer_4));
    set(plot5, 'YData', channel_buffer_5,'XData', 1:numel(channel_buffer_5));
    
    drawnow limitrate;  % Update plot

end