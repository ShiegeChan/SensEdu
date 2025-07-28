function plot_measurements(dist_matrix)
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

    plot(dist_matrix(i, :), 'LineWidth', 0.8, 'Marker', m); hold on;

end
grid on
%ylim([0 0.8])
xlabel("sample index");
ylabel("distance [m]")
legend(mic_name, 'Location', 'best');
title("Microphone distance measurements")
beautify_plot(gcf, 1);
end