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
channel_assigning = [1,1,1,2,2,2];

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
%Here, there is a short loop measuring the muscle activity in a relaxed
%state. It calculates the average voltage for muscles in a relaxed state
%and sets the upper and lower bounds to get the amplitude
% that defines the "flexed" VS the "relaxed" state.

i=0;
fprintf("Ready to start measuring, please relax \n");
robot.delay(1000);

while (i<2*(numValuesToDisplay/ValuesPerChannel)+2)
    % Read from UDP
    Data = read(udpObj, ValuesPerRead, "int16");
    Data = Data';
    %Split up the long Data-block into the respective channels; Defined by
    %Aurix

    for n = 1:6
        channel_buffers(:,n)=[channel_buffers((ValuesPerChannel + 1:end),n) ; Data((n-1)*ValuesPerChannel+1:n*ValuesPerChannel)];
        filtered_buffers(:,n)=[filtered_buffers((ValuesPerChannel + 1:end),n) ; Data((n-1)*ValuesPerChannel+1:n*ValuesPerChannel)];
        filtered_buffers(:,n) = Filter_Buffer(filtered_buffers(:,n),n-1);
    end

    i = i+1;
end

relaxed_averages = zeros(1,6);
upper_bounds = zeros(1,6);
lower_bounds = zeros(1,6);

for n = 1:6
    relaxed_averages(n) = mean(filtered_buffers((end-values_to_check:end),n));

    if (channel_assigning(n)==1)
        upper_bounds(n) = (1+0.01*percent_margin_1)*max(filtered_buffers((end-values_to_check:end),n));
        lower_bounds(n) = (1-0.01*percent_margin_1)*min(filtered_buffers((end-values_to_check:end),n));
    elseif ((channel_assigning(n)==2))
        upper_bounds(n) = (1+0.01*percent_margin_2)*max(filtered_buffers((end-values_to_check:end),n));
        lower_bounds(n) = (1-0.01*percent_margin_2)*min(filtered_buffers((end-values_to_check:end),n));
    else
        fprintf("Wrong assigning of channels to players 1 and 2. \n")
    end

end


fprintf("\n\n\n\n\n Finished calibrating, ready to go! \n");
i=0; %Counter for optional stopping loop later

%%
% Main loop to continuously receive and update data

while true
     
    % Read from UDP
    Data = read(udpObj, ValuesPerRead, "int16");
    Data = Data';
    
    %Split up the long Data-block into the respective channels; Defined by
    %Aurix
   

    for n = 1:6
        channel_buffers(:,n)=[channel_buffers((ValuesPerChannel + 1:end),n) ; Data((n-1)*ValuesPerChannel+1:n*ValuesPerChannel)];
        filtered_buffers(:,n)=[filtered_buffers((ValuesPerChannel + 1:end),n) ; Data((n-1)*ValuesPerChannel+1:n*ValuesPerChannel)];
        filtered_buffers(:,n) = Filter_Buffer(filtered_buffers(:,n),n-1);
    end

    
    

    % Update plot data for each channel
    for n = 1:6
    set(plot_array(n), 'YData', filtered_buffers(:,n),'XData', x_data);
    end

    %FOR TEST
    %set(plot_array(6), 'YData', filtered_buffers(:,3),'XData', x_data);
    
    drawnow limitrate;  % Update plot only every 50 milliseconds to save computing performance

    for n = connected_muscles
    trigger_keyboard(n-1,filtered_buffers(:,n),upper_bounds(n),lower_bounds(n));
    end


    %Optional for pausing and recording data
    %i=i+1;
    %fprintf("%d \n",i);
    %if (i==400)
        
         %fprintf("pause \n");
    %end
end



%%
%The following function defines the keyboard reaction to the measured
%voltages. Different muscle groups that are measured can all be input into
%the function and the correct key will be pressed as a result. The function
%needs the number of the muscle, the last measured values as well as the
%upper and lower bounds, measured from the resting position of the muscle.
function trigger_keyboard (muscle_number, voltage, upperbound, lowerbound)

    %fprintf("Bound check for triggering the keyboard started \n");

    import java.awt.Robot; %The local function has no access to even the imported files
    import java.awt.event.*;

    global robot
    global key_set

    %Here can be set which key should be pressed when muscle groups 0,1,2,3 are
    %flexed
    key0 = KeyEvent.VK_UP;
    key1 = KeyEvent.VK_W;
    key2 = KeyEvent.VK_S;
    key3 = KeyEvent.VK_DOWN;
    key4 = KeyEvent.VK_LEFT;
    key5 = KeyEvent.VK_LEFT;

    %NOTE: An amplitude-based decision if the muscle is flexed is more reliable. For that,
    %min() and max() are used on the last values of
    %voltage, meaning voltage(end-XXX:end). That is compared with
    %(upperbound-lowerbound)
    big_amplitude=false;
    if (abs(max(voltage(end-4000:end)-min(voltage(end-4000:end)))) > abs(upperbound-lowerbound))
        big_amplitude = true;
        fprintf("Signal %d has a big amplitude \n", muscle_number);
    else
        %fprintf("Signal %d has a normal amplitude \n", muscle_number);
    end
    
    %If the amplitude is big and the key is not set
    if (big_amplitude & (key_set(1+muscle_number)==false) )

        if (muscle_number==0)
            fprintf("Pressing key 0 \n");
            robot.keyPress(key0);
            key_set(1+muscle_number) = true;
        elseif (muscle_number==1)
            fprintf("Pressing key 1 \n");
            robot.keyPress(key1);
            key_set(1+muscle_number) = true;
        elseif (muscle_number==2)
            fprintf("Pressing key 2 \n");
            robot.keyPress(key2);
            key_set(1+muscle_number) = true;
        elseif (muscle_number==3)
            fprintf("Pressing key 3 \n");
            robot.keyPress(key3);
            key_set(1+muscle_number) = true;
        elseif (muscle_number==4)
            fprintf("Pressing key 4 \n");
            robot.keyPress(key4);
            key_set(1+muscle_number) = true;
        elseif (muscle_number==5)
            fprintf("Pressing key 5 \n");
            robot.keyPress(key5);
            key_set(1+muscle_number) = true;
        else
            fprintf("No key defined for this muscle group \n");
        end

    %If the amplitude is not big but the key is still set
    elseif (~big_amplitude & (key_set(1+muscle_number)==true))

        if (muscle_number==0)
            fprintf("Releasing key 0 \n");
            robot.keyRelease(key0);
            key_set(1+muscle_number) = false;
        elseif (muscle_number==1)
            fprintf("Releasing key 1 \n");
            robot.keyRelease(key1);
            key_set(1+muscle_number) = false;
        elseif (muscle_number==2)
            fprintf("Releasing key 2 \n");
            robot.keyRelease(key2);
            key_set(1+muscle_number) = false;
        elseif (muscle_number==3)
            fprintf("Releasing key 3 \n");
            robot.keyRelease(key3);
            key_set(1+muscle_number) = false;
        elseif (muscle_number==4)
            fprintf("Releasing key 4 \n");
            robot.keyRelease(key4);
            key_set(1+muscle_number) = false;
        elseif (muscle_number==5)
            fprintf("Releasing key 5 \n");
            robot.keyRelease(key5);
            key_set(1+muscle_number) = false;
        else
            fprintf("No key defined for this muscle group \n");
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

