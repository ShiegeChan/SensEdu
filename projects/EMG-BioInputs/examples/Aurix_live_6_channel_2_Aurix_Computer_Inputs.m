clc;
clear;
close all;

%%
%This part of the code is for initializing the computer inputs
import java.awt.Robot;
import java.awt.event.*;
global robot
robot = java.awt.Robot();
robot.delay(2000);

global key_set
key_set = [false,false,false,false,false,false]; %Keys number 0,1,2,3 are either pressed or not pressed
%Carefull! MATLAB Indices start at 1, always adress key_set(1+muscle_number)



%%

two_Aurix_used = false; %Change accordingly
six_channels_single_Aurix = false;

% UDP setup
port = 5005;
udpObj = udpport("LocalPort", port);

if(two_Aurix_used)
    port2 = 5004;
    udpObj2 = udpport("LocalPort", port2);
end

valuesPerRead = 5000;  % Number of values per read

% Parameters
syncValue = 2056;
numValuesToDisplay = 156250;

channel_buffer_0 = zeros(numValuesToDisplay, 1, 'int16');
channel_buffer_1 = zeros(numValuesToDisplay, 1, 'int16');
channel_buffer_2 = zeros(numValuesToDisplay, 1, 'int16');

channel_buffer_3 = zeros(numValuesToDisplay, 1, 'int16');
channel_buffer_4 = zeros(numValuesToDisplay, 1, 'int16');
channel_buffer_5 = zeros(numValuesToDisplay, 1, 'int16');

% Initialize buffers for each channel
channelData0 = zeros(numValuesToDisplay, 1, 'int16');
channelData1 = zeros(numValuesToDisplay, 1, 'int16');
channelData2 = zeros(numValuesToDisplay, 1, 'int16');

channelData3 = zeros(numValuesToDisplay, 1, 'int16');
channelData4 = zeros(numValuesToDisplay, 1, 'int16');
channelData5 = zeros(numValuesToDisplay, 1, 'int16');

disp('Waiting for sync frame...');

%%
% Create figure and axes
fig = figure;
y_low = -1000; %Default -32768
y_high = 20000; %Default 32767
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

if(two_Aurix_used|six_channels_single_Aurix)
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
end


%%
%Here, there is a short loop measuring the muscle activity in a relaxed
%state. It calculates the average voltage for muscles in a relaxed state
%and sets the upper and lower bounds to get the amplitude
% that defines the "flexed" VS the "relaxed" state.

i=0;
fprintf("Ready to start measuring, please relax \n");
robot.delay(1000);

while (i<(numValuesToDisplay/valuesPerRead)+2)
    % Read a frame from UDP
    data = read(udpObj, valuesPerRead, "int16");
    
    % Check if it's a sync frame
    if all(data == syncValue)
        % Read data frames for each channel
        channelData0 = read(udpObj, valuesPerRead, "int16");
        channelData0= channelData0(:);
        channelData1 = read(udpObj, valuesPerRead, "int16");
        channelData1= channelData1(:);
        channelData2 = read(udpObj, valuesPerRead, "int16");
        channelData2= channelData2(:);
      
        % Update channel_buffer_0 by shifting out the oldest data and appending the new data
        channel_buffer_0 = [channel_buffer_0(valuesPerRead+1:end); channelData0(1:valuesPerRead)];
        channel_buffer_1 = [channel_buffer_1(valuesPerRead+1:end); channelData1(1:valuesPerRead)];
        channel_buffer_2 = [channel_buffer_2(valuesPerRead+1:end); channelData2(1:valuesPerRead)];

        i = i+1;
    end

    if(two_Aurix_used)
        % Read a frame from UDP
        data2 = read(udpObj2, valuesPerRead, "int16");
        
        % Check if it's a sync frame
        if all(data2 == syncValue)
            % Read data frames for each channel
            channelData3 = read(udpObj2, valuesPerRead, "int16");
            channelData3= channelData3(:);
            channelData4 = read(udpObj2, valuesPerRead, "int16");
            channelData4= channelData4(:);
            channelData5 = read(udpObj2, valuesPerRead, "int16");
            channelData5= channelData5(:);
    
    
          
            % Update channel_buffer by shifting out the oldest data and appending the new data
            channel_buffer_3 = [channel_buffer_3(valuesPerRead+1:end); channelData3(1:valuesPerRead)];
            channel_buffer_4 = [channel_buffer_4(valuesPerRead+1:end); channelData4(1:valuesPerRead)];
            channel_buffer_5 = [channel_buffer_5(valuesPerRead+1:end); channelData5(1:valuesPerRead)];    
    
        end
    end

end

%Filter_Buffer 50 Hz notch Filter Johannes
    channel_buffer_0 = Filter_Buffer(channel_buffer_0);
    channel_buffer_1 = Filter_Buffer(channel_buffer_1);
    channel_buffer_2 = Filter_Buffer(channel_buffer_2);

    channel_buffer_3 = Filter_Buffer(channel_buffer_0);
    channel_buffer_4 = Filter_Buffer(channel_buffer_1);
    channel_buffer_5 = Filter_Buffer(channel_buffer_2);

    relaxed_average_0 = mean(channel_buffer_0(end-1000:end));
    upperbound_0 = 1.05*max(channel_buffer_0(end-1000:end));
    lowerbound_0 = min(channel_buffer_0(end-1000:end))*0.95;

    relaxed_average_1 = mean(channel_buffer_1(end-1000:end));
    upperbound_1 = 1.05*max(channel_buffer_1(end-1000:end));
    lowerbound_1 = min(channel_buffer_1(end-1000:end))*0.95;

    relaxed_average_2 = mean(channel_buffer_2(end-1000:end));
    upperbound_2 = 1.05*max(channel_buffer_2(end-1000:end));
    lowerbound_2 = min(channel_buffer_2(end-1000:end))*0.95;

    relaxed_average_3 = mean(channel_buffer_3(end-1000:end));
    upperbound_3 = 1.05*max(channel_buffer_3(end-1000:end));
    lowerbound_3 = min(channel_buffer_3(end-1000:end))*0.95;

    relaxed_average_4 = mean(channel_buffer_4(end-1000:end));
    upperbound_4 = 1.05*max(channel_buffer_4(end-1000:end));
    lowerbound_4 = min(channel_buffer_4(end-1000:end))*0.95;

    relaxed_average_5 = mean(channel_buffer_5(end-1000:end));
    upperbound_5 = 1.05*max(channel_buffer_5(end-1000:end));
    lowerbound_5 = min(channel_buffer_5(end-1000:end))*0.95;



i=0; %Counter for optional stopping loop later

%%
% Main loop to continuously receive and update data
while true
    % Read a frame from UDP
    data = read(udpObj, valuesPerRead, "int16");
    
    % Check if it's a sync frame
    if all(data == syncValue)
        % Read data frames for each channel
        channelData0 = read(udpObj, valuesPerRead, "int16");
        channelData0= channelData0(:);
        channelData1 = read(udpObj, valuesPerRead, "int16");
        channelData1= channelData1(:);
        channelData2 = read(udpObj, valuesPerRead, "int16");
        channelData2= channelData2(:);
                
        % Update channel_buffer_0 by shifting out the oldest data and appending the new data
        channel_buffer_0 = [channel_buffer_0(valuesPerRead+1:end); channelData0(1:valuesPerRead)];
        channel_buffer_1 = [channel_buffer_1(valuesPerRead+1:end); channelData1(1:valuesPerRead)];
        channel_buffer_2 = [channel_buffer_2(valuesPerRead+1:end); channelData2(1:valuesPerRead)];

        % Update plot data for each channel
        set(plot0, 'YData', channel_buffer_0,'XData', 1:numel(channel_buffer_0));
        set(plot1, 'YData', channel_buffer_1,'XData', 1:numel(channel_buffer_1));
        set(plot2, 'YData', channel_buffer_2,'XData', 1:numel(channel_buffer_2));
        
        drawnow limitrate;  % Update plot

        %trigger_keyboard(0,channel_buffer_0,upperbound_0,lowerbound_0);
        trigger_keyboard(1,channel_buffer_1,upperbound_1,lowerbound_1); 
        %trigger_keyboard(2,channel_buffer_2,upperbound_2,lowerbound_2);
                
    end


    if(two_Aurix_used)
        % Read a frame from UDP
        data2 = read(udpObj2, valuesPerRead, "int16");
        
        % Check if it's a sync frame
        if all(data2 == syncValue)
            % Read data frames for each channel
            channelData3 = read(udpObj2, valuesPerRead, "int16");
            channelData3= channelData3(:);
            channelData4 = read(udpObj2, valuesPerRead, "int16");
            channelData4= channelData4(:);
            channelData5 = read(udpObj2, valuesPerRead, "int16");
            channelData5= channelData5(:);    
          
            % Update channel_buffer by shifting out the oldest data and appending the new data
            channel_buffer_3 = [channel_buffer_3(valuesPerRead+1:end); channelData3(1:valuesPerRead)];
            channel_buffer_4 = [channel_buffer_4(valuesPerRead+1:end); channelData4(1:valuesPerRead)];
            channel_buffer_5 = [channel_buffer_5(valuesPerRead+1:end); channelData5(1:valuesPerRead)]; 

            % Update plot data for each channel
            set(plot3, 'YData', channel_buffer_3,'XData', 1:numel(channel_buffer_3));
            set(plot4, 'YData', channel_buffer_4,'XData', 1:numel(channel_buffer_4));
            set(plot5, 'YData', channel_buffer_5,'XData', 1:numel(channel_buffer_5));
            
            drawnow limitrate;  % Update plot

            %trigger_keyboard(3,channel_buffer_3,upperbound_3,lowerbound_3);
            %trigger_keyboard(4,channel_buffer_4,upperbound_4,lowerbound_4);
            %trigger_keyboard(5,channel_buffer_5,upperbound_5,lowerbound_5);
            
        end
    end
    
    %Try spmd for parallel reading
    %flush(udpObj);
    %flush(udpObj2);

    %Optional for pausing and recording data
    i=i+1;
    fprintf("%d \n",i);
    %if (i==100)
    %     channel_buffer_1 = Filter_Buffer(channel_buffer_1);
    %     % Update plot data for each channel
    %     %set(plot0, 'YData', channel_buffer_0,'XData', 1:numel(channel_buffer_0));
    %     %set(plot1, 'YData', channel_buffer_1,'XData', 1:numel(channel_buffer_1));
    %     set(plot2, 'YData', channel_buffer_1,'XData', 1:numel(channel_buffer_1));
    % 
    %     drawnow limitrate;  % Update plot
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
    key0 = KeyEvent.VK_LEFT;
    key1 = KeyEvent.VK_RIGHT;
    key2 = KeyEvent.VK_LEFT;
    key3 = KeyEvent.VK_LEFT;
    key4 = KeyEvent.VK_LEFT;
    key5 = KeyEvent.VK_LEFT;


    %filter Johannes begin

    voltage = Filter_Buffer(voltage); %Notch filter 50% the data

    %filter_end

    %NOTE: An amplitude-based decision if the muscle is flexed is more reliable. For that,
    %min() and max() are used on the last values of
    %voltage, meaning voltage(end-XXX:end). That is compared with
    %(upperbound-lowerbound)
    big_amplitude=false;
    if (abs(max(voltage(end-5000:end)-min(voltage(end-5000:end)))) > abs(upperbound-lowerbound))
        big_amplitude = true;
        fprintf("Signal %d has a big amplitude \n", muscle_number);
    else
        fprintf("Signal %d has a normal amplitude \n", muscle_number);
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
function [OutBuffer] = Filter_Buffer(InBuffer)
%Notch filter 50Hz the InBuffer and put the result in OutBuffer
%Johannes Kohlhaas


Fs = 156250;   % Sampling frequency
Fnyq = Fs/2; % Nyquist frequency is half the sampling frequency

F0 = 50;   % Interference is at 50 Hz
BW = 2;    % Choose a bandwidth factor of 2 Hz
%[num1,den1] = designNotchPeakIIR(Response="notch",CenterFrequency=F0/Fnyq,Bandwidth=BW/Fnyq,FilterOrder=2);
num1 = [0.999959789230987,-1.999915536035921,0.999959789230979];
den1 = [1,-1.999915536035921,0.999919578461967];
OutBuffer = filter(num1,den1,InBuffer);


end

