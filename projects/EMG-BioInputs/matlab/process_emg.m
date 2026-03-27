function process_emg(data, fs, enable_plots)
    b = fir1(151, [20 450]/fs/2, 'bandpass');
    delay = (length(b) - 1)/2;
    filtered_data = filter(b, 1, data);
    filtered_data_a = filtered_data((delay + 1):end);

    figure;
    hold on;
    plot(filtered_data);
    plot(filtered_data_a);
end