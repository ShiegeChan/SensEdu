% with that script you can generate wave for xcorr algorithm
% dac wave gen
 f = 32e3; % wave freq
 fs = 244e3; % adc sampling rate

 t = 0:(0.000000001):0.000315;
 ts = 0:(1/fs):0.000315;

 y = (sin(2*pi*f*t)/2 + 0.5).*65535;
 ys = (sin(2*pi*f*ts)/2 + 0.5).*65535;
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
 plot(ts, ys);
 hold on;
 plot(ts, round(ys));
 hold off;
 legend("y", "ys", "round ys")
 %save("Measurements\test_xcorr_dac_wave.mat", "ys");