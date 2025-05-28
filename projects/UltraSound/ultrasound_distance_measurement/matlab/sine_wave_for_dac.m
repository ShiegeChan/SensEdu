clear;
close all;
clc;
sine_wave_hex = ["000", "00A",'027','058','09c','0f2','159','1d1','258','2ed','38e','43a','4f0','5ad','670','737', '800','8c8','98f','a52','b0f','bc5','c71','d12','da7','e2e','ea6','f0d','f63','fa7','fd8','ff5','fff','ff5','fd8','fa7','f63','f0d','ea6','e2e','da7','d12','c71','bc5','b0f','a52','98f','8c8', '800','737','670','5ad','4f0','43a','38e','2ed','258','1d1','159','0f2','09c','058','027','00a'];
sine_wave = hex2dec(sine_wave_hex);
sine_wave_v = (3.3/4096) .* sine_wave;
figure;
plot(sine_wave_v);



f = 32e3;
t_end = 1/f;
t = 0:t_end/64:t_end; 
sig = 0.4 * sin(2*pi*f*t) + 1.65;

sig_dig = sig .* (4096 / 3.3);

figure;
plot(t, sig);

sig_dig_hex = dec2hex(round(sig_dig));

lut_sig_dig = "";

 lut_size = size(sig_dig_hex, 1);
 for i = 1:lut_size
     lut_sig_dig = strcat(lut_sig_dig, "0x", sig_dig_hex(i, :), ", ");
     if mod(i, 16) == 0
         lut_sig_dig = strcat(lut_sig_dig, "\n");
     end
 end
 fprintf(lut_sig_dig);
 disp(lut_size);

