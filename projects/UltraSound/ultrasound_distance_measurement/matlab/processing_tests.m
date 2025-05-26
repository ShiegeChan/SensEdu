for i = 1:50
    figure;
    subplot(4,1,1);
    plot(dist_matrix(5,i), '*-', 'LineWidth', 6);
    subplot(4,1,2);
    plot(detail_info(13,:,i));
    subplot(4,1,3);
    plot(detail_info(14,:,i));
    subplot(4,1,4);
    plot(detail_info(15,:,i));
end
%% processing
raw = detail_info(13,:,15);
filtered_sig = filter(coefficients, 1, raw);
figure;
plot(raw); hold on; plot(filtered_sig);


%%


s = detail_info(14,:,21);
s_xcor = detail_info(15,:,21);
figure;
plot(s);
%hold on;
%plot(s_xcor);

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
xlim([0 Fs/2]);      % Display up to Nyquist frequency
grid on;

