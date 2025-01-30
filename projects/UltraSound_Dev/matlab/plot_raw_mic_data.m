function plot_raw_mic_data(data_mic1, data_mic2, data_mic3, data_mic4)
    subplot(4,1,1)
    plot(data_mic1);
    ylim([0, 65535]);
    xlim([0, length(data_mic1)])
    xlabel("Sample #");
    ylabel("ADC1 channel 5 16bit value");
    title("Microphone 1 data")
    grid on;

    subplot(4,1,2)
    plot(data_mic2)
    ylim([0, 65535]);
    xlim([0, length(data_mic2)])
    xlabel("Sample #");
    ylabel("ADC1 channel 9 16bit value");
    title("Microphone 2 data");
    grid on;

    subplot(4,1,3)
    plot(data_mic3)
    ylim([0, 65535]);
    xlim([0, length(data_mic3)])
    xlabel("Sample #");
    ylabel("ADC2 channel 1 16bit value");
    title("Microphone 3 data");
    grid on;

    subplot(4,1,4)
    plot(data_mic4)
    ylim([0, 65535]);
    xlim([0, length(data_mic4)])
    xlabel("Sample #");
    ylabel("ADC2 channel 6 16bit value");
    title("Microphone 4 data");
    grid on;
    
end