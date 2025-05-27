clc;
clear;
close all;

%% UDP Connection
Port = 5005;
NumberOfChannels = 6;
ValuesPerChannel = 2000; % Number of values expected per channel, defined by Aurix
ValuesPerRead = ValuesPerChannel * NumberOfChannels; % Number of values expected per read, defined by Aurix
UdpObj = udpport("LocalPort", Port);

%% Choose Operation Mode: 0 = Phaseshift, 1 = Waveforms
Waveforms = 1;

%% Parameters
SampleFrequency = 156250;
DownSampleFrequency = 12500;
OffsetComp0 = -690;
OffsetComp1 = -730;
OffsetComp2 = -880;
LPFreq = 70;
AmpLimit = 0.003;
FilterOrder = 2;
GroupDelay = 500;
DispValues = DownSampleFrequency - GroupDelay;
CarrierFrequency = 33333;
count = 0;
if (Waveforms == 1)
    AmpLimit = 33000;
    DispValues = SampleFrequency;
end

%% Preallocate Buffer Sizes
Data = zeros(ValuesPerRead, 1);
DataBuffer = double(zeros(6, SampleFrequency));
PhaseShift = double(zeros(6, DownSampleFrequency - GroupDelay));
PlotHandler = gobjects(NumberOfChannels, 1);  % gobjects creates an array of graphics objects

%% Precalculate Values and Parameters
t = (0:1/SampleFrequency:(1 - 1/SampleFrequency));
Exp_Shift = exp(-1i * 2 * pi * CarrierFrequency * t);
[p, q] = rat(DownSampleFrequency / SampleFrequency);

%% Define Lowpass
LPFiltIIR1 = designfilt('lowpassiir', 'FilterOrder', FilterOrder, 'HalfPowerFrequency', LPFreq, 'SampleRate', SampleFrequency);
LPFiltIIR2 = designfilt('lowpassiir', 'FilterOrder', FilterOrder, 'HalfPowerFrequency', 50, 'SampleRate', DownSampleFrequency);

%% Prepare Plot
fig = figure;
sgtitle('Heartbeat Measurement FHTW');
subplot_definitions = [1:NumberOfChannels; 1:NumberOfChannels];
for i = 1:NumberOfChannels
    subplot(NumberOfChannels, 1, i)
    PlotHandler(i) = plot(0);
    title(['Channel ', num2str(i - 1)]);
    xlabel('Samples');
    ylabel('Amplitude');
    xlim([0, DispValues]);
    ylim([-AmpLimit, AmpLimit]);
    grid on;
end


%% Main loop to read and process incoming data
disp('Program started...');
while ishandle(fig) % Check if figure exists
    % Read Data from UDP Port and eliminate DC Offset
    Data = read(UdpObj, ValuesPerRead, "int16");
    % Split Data in Channels and apply offset compensation
    DataBuffer = Split_Data(Data, DataBuffer, ValuesPerChannel);
    count = count + 1;
    if (count == 5)
        count = 0;
        if (Waveforms == 1)
            Plot_Waveforms(DataBuffer,PlotHandler);
        else
            Calc_Draw(DataBuffer,PhaseShift,Exp_Shift,LPFiltIIR1,p,q,LPFiltIIR2,GroupDelay,DownSampleFrequency,PlotHandler);
        end
    end

    if (ishandle(fig) == 0)
        disp('Figure window has been closed.');
        break;
    end
end

close all;
clc;
% Close the parallel pool
disp('Program successfully closed.');


function Phase_Shift = demod_filter(DataBuffer,Exp_Shift,LPFiltIIR1,p,q,LPFiltIIR2,GroupDelay,DownSampleFrequency)
% Shift DataBuffer to Baseband by multiplying with complex signal
Baseband = DataBuffer .* Exp_Shift;
% Apply first Lowpass Filter.
Baseband = filter(LPFiltIIR1, Baseband);
% Resample to 12,5kHz
Baseband = resample(Baseband, p, q);
% Extract I and Q Components
[I0, Q0] = deal(real(Baseband), imag(Baseband));
% Calculate the Angle
Phase_Shift = unwrap(atan2(Q0, I0));
% Calculate the Gradient the Phase values
Phase_Shift = gradient(Phase_Shift);
% Apply second Lowpass Filter.
Phase_Shift = filter(LPFiltIIR2, Phase_Shift);
% Correct for Group Delay of Lowpass Filter
Phase_Shift(1:GroupDelay) = 0;
% Finetune offset Compensation
Phase_Shift = Phase_Shift(GroupDelay+1:DownSampleFrequency)-mean(Phase_Shift);
end

function DataBuffer = Split_Data(Data,DataBuffer,ValuesPerChannel)
DataBuffer(1,:) = [DataBuffer(1,ValuesPerChannel + 1:end), Data(1:ValuesPerChannel)];
DataBuffer(2,:) = [DataBuffer(2,ValuesPerChannel + 1:end), Data(ValuesPerChannel+1:ValuesPerChannel*2)];
DataBuffer(3,:) = [DataBuffer(3,ValuesPerChannel + 1:end), Data(ValuesPerChannel*2+1:ValuesPerChannel*3)];
DataBuffer(4,:) = [DataBuffer(4,ValuesPerChannel + 1:end), Data(ValuesPerChannel*3+1:ValuesPerChannel*4)];
DataBuffer(5,:) = [DataBuffer(5,ValuesPerChannel + 1:end), Data(ValuesPerChannel*4+1:ValuesPerChannel*5)];
DataBuffer(6,:) = [DataBuffer(6,ValuesPerChannel + 1:end), Data(ValuesPerChannel*5+1:ValuesPerChannel*6)];
end


function  Calc_Draw(DataBuffer,PhaseShift,Exp_Shift,LPFiltIIR1,p,q,LPFiltIIR2,GroupDelay,DownSampleFrequency,PlotHandler)
%disp('Function called');
PhaseShift(1,:) = demod_filter(DataBuffer(1,:),Exp_Shift,LPFiltIIR1,p,q,LPFiltIIR2,GroupDelay,DownSampleFrequency);
PhaseShift(2,:) = demod_filter(DataBuffer(2,:),Exp_Shift,LPFiltIIR1,p,q,LPFiltIIR2,GroupDelay,DownSampleFrequency);
PhaseShift(3,:) = demod_filter(DataBuffer(3,:),Exp_Shift,LPFiltIIR1,p,q,LPFiltIIR2,GroupDelay,DownSampleFrequency);
PhaseShift(4,:) = demod_filter(DataBuffer(4,:),Exp_Shift,LPFiltIIR1,p,q,LPFiltIIR2,GroupDelay,DownSampleFrequency);
PhaseShift(5,:) = demod_filter(DataBuffer(5,:),Exp_Shift,LPFiltIIR1,p,q,LPFiltIIR2,GroupDelay,DownSampleFrequency);
PhaseShift(6,:) = demod_filter(DataBuffer(6,:),Exp_Shift,LPFiltIIR1,p,q,LPFiltIIR2,GroupDelay,DownSampleFrequency);
set(PlotHandler(1), 'XData', 1:numel(PhaseShift(1,:)), 'YData', PhaseShift(1,:));
set(PlotHandler(2), 'XData', 1:numel(PhaseShift(2,:)), 'YData', PhaseShift(2,:));
set(PlotHandler(3), 'XData', 1:numel(PhaseShift(3,:)), 'YData', PhaseShift(3,:));
set(PlotHandler(4), 'XData', 1:numel(PhaseShift(4,:)), 'YData', PhaseShift(4,:));
set(PlotHandler(5), 'XData', 1:numel(PhaseShift(5,:)), 'YData', PhaseShift(5,:));
set(PlotHandler(6), 'XData', 1:numel(PhaseShift(6,:)), 'YData', PhaseShift(6,:));
drawnow limitrate;
end

function  Plot_Waveforms(DataBuffer,PlotHandler)
set(PlotHandler(1), 'XData', 1:numel(DataBuffer(1,:)), 'YData', DataBuffer(1,:));
set(PlotHandler(2), 'XData', 1:numel(DataBuffer(2,:)), 'YData', DataBuffer(2,:));
set(PlotHandler(3), 'XData', 1:numel(DataBuffer(3,:)), 'YData', DataBuffer(3,:));
set(PlotHandler(4), 'XData', 1:numel(DataBuffer(4,:)), 'YData', DataBuffer(4,:));
set(PlotHandler(5), 'XData', 1:numel(DataBuffer(5,:)), 'YData', DataBuffer(5,:));
set(PlotHandler(6), 'XData', 1:numel(DataBuffer(6,:)), 'YData', DataBuffer(6,:));
end

