close all;
clear;
clc;

% with that script you can generate wave for xcorr algorithm
% dac wave gen
 f = 32e3; % wave freq
 fs = 244e3; % adc sampling rate

 t = 0:(0.000000001):0.000315;
 ts = 0:(1/fs):0.000315;
 
 y = (generate_speaker_signal(f, t, 10, 0.4) .* 65535) + 32767; % shifting the signal from negative starting range to 0 start range
 ys = (generate_speaker_signal(f, ts, 10, 0.4) .* 65535) + 32767;

 res = dec2hex(round(ys));

 lut = "";

 lut_size = size(res, 1);
 for i = 1:lut_size
     lut = strcat(lut, "0x", res(i, :), ", ");
     if mod(i, 16) == 0
         lut = strcat(lut, "\n");
     end
 end
 fprintf(lut);
 disp(lut_size);

 figure;
 plot(t, y);
 hold on;
 plot(ts, ys, '.-');
 hold on;
 plot(ts, round(ys));
 hold off;
 legend("y", "ys", "round ys")

 save("test_xcorr_dac_wave.mat", "ys");