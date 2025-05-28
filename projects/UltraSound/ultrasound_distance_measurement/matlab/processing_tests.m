close all;
for i = 1:50
    figure;
    subplot(3,1,1);
    %plot(dist_matrix(6,i), '*-', 'LineWidth', 6);
    plot(detail_info(4,:,i));
    title("Raw signal from MIC 1");
    subplot(3,1,2);
    plot(detail_info(5,:,i));
    title("Processed signal from MIC 1")
    subplot(3,1,3);
    plot(detail_info(6,:,i));
    title("Cross correlation result from MIC 1")
    xlabel("Sample number")
end
beautify_plot(gcf, 1)
%%
mic_name = {"MIC 1", "MIC 2", "MIC 3", "MIC 4", "MIC 5", "MIC 6"};
figure
MIC_NUM = 6;
for i = 1:MIC_NUM
    plot(time_axis, dist_matrix(i, :), 'LineWidth', 2); hold on;
end
ylim([0 1])
xlim([0 time_axis(end)])
grid on
xlabel("time [s]");
ylabel("distance [m]")
legend(mic_name);
title("Microphone distance measurements")

%% processing
% raw = detail_info(13,:,15);
% filtered_sig = filter(coefficients, 1, raw);
% figure;
% plot(raw); hold on; plot(filtered_sig);
% 
% 
% %
close all
s = detail_info(2,:,20);
s_xcor = detail_info(3,:,20);
% figure;
% plot(s);
% %hold on;
% %plot(s_xcor);

Fs = 250e3;
T = 1/Fs;
L = 2048;

% compute fft
S = fft(s);
P2 = abs(S/L);
P1 = P2(1:L/2+1);
P1(2:end-1) = 2*P1(2:end-1);

% freq axis
f_axis = Fs*(0:(L/2))/L; % Frequency range: 0 Hz to Fs/2 (Nyquist)
figure;
%plot
plot(f_axis, P1);
title('Single-Sided Amplitude Spectrum');
xlabel('Frequency (Hz)');
ylabel('Amplitude');
xlim([0 Fs/4]);      % Display up to Nyquist frequency
grid on;

hold on;
%%
s = detail_info(2,:,11);
s_xcor = detail_info(3,:,11);
% figure;
% plot(s);
% %hold on;
% %plot(s_xcor);

Fs = 250e3;
T = 1/Fs;
L = 2048;

% compute fft
S = fft(s);
P2 = abs(S/L);
P1 = P2(1:L/2+1);
P1(2:end-1) = 2*P1(2:end-1);

% freq axis
f_axis = Fs*(0:(L/2))/L; % Frequency range: 0 Hz to Fs/2 (Nyquist)
%plot
plot(f_axis, P1);
title('Single-Sided Amplitude Spectrum');
xlabel('Frequency (Hz)');
ylabel('Amplitude');
xlim([0 Fs/4]);      % Display up to Nyquist frequency
grid on;