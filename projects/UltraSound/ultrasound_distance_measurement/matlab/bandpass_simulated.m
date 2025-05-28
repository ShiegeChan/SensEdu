close all;
filtered_mic_data = zeros(MIC_NUM, DATA_LENGTH, ITERATIONS);
load("filter_taps.mat");
%load("cardboard_semi_problematic_data_15-May-2025_11-56-28.mat");
% cnt = 0;
% for i=1:ITERATIONS
%     for j = 1:3:MIC_NUM*3
%         cnt = cnt+1;
%         filtered_mic_data(cnt,:,i) = filter(coefficients, 1, detail_info(j,:,i));
%     end
% end
iter_num = 30;
mic1 = detail_info(1,:,iter_num);

% convert from -1 to 1
scaled_mic1 = (2.0 * mic1) ./ 65535.0 - 1.0;

% filter
filt = filter(coefficients', 1, scaled_mic1);

% plot
figure;
subplot(1,3,1);
plot(mic1);
subplot(1,3,2);
plot(scaled_mic1);
subplot(1,3,3);
plot(filt);
hold on;
plot(detail_info(2,:,iter_num));
legend('modeled filter', 'real system filter')
title("raw vs filtered")
hold off;