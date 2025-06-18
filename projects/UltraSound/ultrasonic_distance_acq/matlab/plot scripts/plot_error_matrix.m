function plot_error_matrix(err_vec, n)


mic_name = {"MIC 1", "MIC 2","MIC 3", "MIC 4", "MIC 8", "MIC 6", "MIC 5", "MIC 7"};
figure
for i = 1:n


    plot(err_vec(i, :), 'LineWidth', 2, 'Marker', '.'); hold on;

end
grid on
xlabel("sample index");
ylabel("value [m]")
legend(mic_name, 'Location', 'best');
title("Error: Measured Distance - Predicted Distance")
beautify_plot(gcf, 1);
end