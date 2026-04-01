function mpt_plot_measurements(dist_matrix, max_peaks)
mic_name = {"MIC 1", "MIC 2","MIC 3", "MIC 4"};
figure
for i = 1:4
    switch i
        case 1
            m = "o";
        case 2
            m = "^";
        case 3
            m = "square";
        case 4
            m = "diamond";
        case 5
            m = "v";
        case 6
            m = "hexagram";
        case 7
            m = "pentagram";
        case 8
            m = ">";
    end

    % adapt for more than 3 peaks
    plot(dist_matrix((i-1)*(max_peaks-1)+i, :), 'LineStyle','none','LineWidth', 0.5, 'Marker', m); hold on;
    if max_peaks>=2
    plot(dist_matrix((i-1)*(max_peaks-1)+i+1, :),  'LineStyle','none','LineWidth', 0.5, 'Marker', m); hold on;
    end
    if max_peaks>=3
    plot(dist_matrix((i-1)*(max_peaks-1)+i+2, :),  'LineStyle','none','LineWidth', 0.5, 'Marker', m); hold on;
    end
end
grid on
% ylim([0.001 2.5])
xlabel("sample index");
ylabel("distance [m]")
legend(mic_name, 'Location', 'best');
title("Microphone distance measurements")
beautify_plot(gcf, 1);
end