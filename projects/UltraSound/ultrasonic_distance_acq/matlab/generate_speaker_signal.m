function [filtered_signal] = generate_speaker_signal(f, t, dac_cycle_num, sigma)
    %GENERATE_SPEAKER_SIGNAL Summary of this function goes here
    
    signal_perfect = sin(2*pi * f * t)/2; % perfect 32 kHz sine wave
    % gaussian filter parameter - standard deviation
    sigma_new = sigma * dac_cycle_num; % adjusting sigma to match the length of the signal
    
    % range of x values for the signal
    x = linspace(-dac_cycle_num, dac_cycle_num, length(signal_perfect));
    
    % compute the values for gaussian window
    gaussian_window = exp(- (x.^2) / (2 * sigma_new^2));
    
    % normalize
    gaussian_window = gaussian_window / max(gaussian_window);
    size(gaussian_window)
    size(signal_perfect)
    % apply the window to the signal
    filtered_signal = signal_perfect .* gaussian_window;
end

