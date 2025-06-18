function plot_outliers_vs_clean(dist_matrix, dist_outliers, MIC_NUM, mic_name)
    figure
    for i = 1:MIC_NUM
        subplot(MIC_NUM/2, MIC_NUM/2, i);
        plot(dist_outliers(i, :), 'LineWidth', 2)
        hold on;
        plot(dist_matrix(i, :), 'LineWidth', 2)
        grid on
        xlabel("sample");
        ylabel("distance [m]");
        legend("Raw Measurements", "Measurements after Outlier Rejection")
        title(mic_name(i));
    end
    beautify_plot(gcf, 1);
end