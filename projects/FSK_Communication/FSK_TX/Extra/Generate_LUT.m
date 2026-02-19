%% Generate_LUT.m
% Generates LUT for bit 0 and bit 1 sine waves
clc;
clear;
close all;

%% Settings
SAMPLE_RATE = 33e3 * 16;
f0 = 31e3; % Target frequency for bit 0
f1 = 35e3; % Target frequency for bit 1
CYCLES_PER_BIT = 8; % Defines how many frequency cycles is sent for one bit (improves stability)

%% Wave Generation
lut_size0 = round(SAMPLE_RATE/f0);
lut_size1 = round(SAMPLE_RATE/f1);

t0 = linspace(0, 1/f0, lut_size0);
t1 = linspace(0, 1/f1, lut_size1);

lut0 = sin(f0*2*pi*t0 - pi/2);
lut1 = sin(f1*2*pi*t1 - pi/2);

lut0_12bit = round(((lut0 + 1)/ 2) * 4095);
lut1_12bit = round(((lut1 + 1)/ 2) * 4095);

%% Plot
figure;
plot(t0, lut0_12bit, 'b');
hold on;
plot(t1, lut1_12bit, 'r');
xlabel("time");
ylabel("DAC value");
legend([string(f0) + " Hz", string(f1) + " Hz"])

%% Print in Hex
hex_char_array0 = "0x" + string(dec2hex(lut0_12bit)); 
hex_char_array1 = "0x" + string(dec2hex(lut1_12bit)); 

fprintf("const uint16_t LUT_BIT0_SIZE = %i * %i;\n", lut_size0, CYCLES_PER_BIT);
fprintf("static uint16_t lut_bit0[LUT_BIT0_SIZE] = {\n");
for i = 1:CYCLES_PER_BIT
    print_hex(hex_char_array0, lut_size0, 9);
    if i ~= CYCLES_PER_BIT
        fprintf(", \n");
    else
        fprintf("\n");
    end
end
fprintf("};\n\n");

fprintf("const uint16_t LUT_BIT1_SIZE = %i * %i;\n", lut_size1, CYCLES_PER_BIT);
fprintf("static uint16_t lut_bit1[LUT_BIT1_SIZE] = {\n");
for i = 1:CYCLES_PER_BIT
    print_hex(hex_char_array1, lut_size1, 9);
    if i ~= CYCLES_PER_BIT
        fprintf(", \n");
    else
        fprintf("\n");
    end
end
fprintf("};\n\n");

%% Functions
function print_hex(array, size, symbols_per_line)
    fprintf("\t");
    for i = 1:size
        fprintf("%s", array(i));
        if i ~= size
            fprintf(", ");
        end
        if (mod(i, symbols_per_line) == 0)
            fprintf("\n\t");
        end
    end
end