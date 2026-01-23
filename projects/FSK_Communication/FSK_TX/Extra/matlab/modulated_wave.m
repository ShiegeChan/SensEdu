clc;
clear;
close all;

% Parameters
sampleRate =  32000;         % Fixed size for both
f0 = 30000;                  % Target frequency for bit 0
f1 = 40000;                  % Target frequency for bit 1
LUT_SIZE_0 = round(64*sampleRate/f0);  % LUT size for bit 0
LUT_SIZE_1 = round(64*sampleRate/f1);  % LUT size for bit 1


% % Calculate how many cycles fit in 64 samples
% cycles_f0 = (f0 * LUT_SIZE) / sampleRate_0;  % 6.4 cycles
% cycles_f1 = (f1 * LUT_SIZE) / sampleRate_1;  % 12.8 cycles

% Generate LUTs with multiple cycles
t_0 = linspace(0, 1/30000, LUT_SIZE_0);
LUT_f0 = sin(f0*2*pi*t_0-pi/2);

t_1 = linspace(0, 1/40000, LUT_SIZE_1);
LUT_f1 = sin(f1*2*pi*t_1-pi/2);

% Scale to 0-4095
LUT_f0_scaled = round((LUT_f0 + 1) * 2047.5); % +1 because arduino dont
LUT_f1_scaled = round((LUT_f1 + 1) * 2047.5); % send negative numbers

hex_char_array_0 = "0x" + string(dec2hex(LUT_f0_scaled)); 
hex_char_array_1 = "0x" + string(dec2hex(LUT_f1_scaled)); 


%% Print in hexadecimal %%
fprintf(' First LUT: \n');
for i = 1:68
    fprintf('%s, ', hex_char_array_0(i));
    if (mod(i, 8) == 0)
        fprintf('\n');
    end
end

fprintf('\nSecond LUT: ');
for i = 1:51
    fprintf('%s, ', hex_char_array_1(i));
    if (mod(i, 8) == 0)
        fprintf('\n');
    end
end

fprintf('\n');
plot(t_0, LUT_f0_scaled, 'b')
hold on
plot(t_1, LUT_f1_scaled, 'r')