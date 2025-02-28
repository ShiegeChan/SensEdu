function [y] = generate_speaker_sig(t)
%GENERATE_SPEAKER_SIG Summary of this function goes here
%   Detailed explanation goes here

f = 32e3;
signal_perfect = sin((2*pi*f*t));
figure;
plot(signal_perfect);
% parameters
sigma = 0.4; % standard deviation
filter_size = round(sigma * length(t));

% create gaussian kernel
gaussian_kernel = fspecial("gaussian", [1, filter_size], sigma);
figure;
plot(gaussian_kernel);
% normalize the kernel
gaussian_kernel = gaussian_kernel / sum(gaussian_kernel);
hold on;
plot(gaussian_kernel); hold off;
% apply to the signal using convolution
signal_filtered = conv(signal_perfect, gaussian_kernel, "same");
signal_filtered = (max(signal_filtered)-signal_filtered) / (max(signal_filtered) - min(signal_filtered));

signal_filtered = signal_filtered .* signal_perfect;
figure;
plot(signal_filtered);
y = signal_filtered;

end

