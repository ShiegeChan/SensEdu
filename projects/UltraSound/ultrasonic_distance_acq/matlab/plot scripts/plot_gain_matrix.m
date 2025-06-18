function plot_gain_matrix(data)
figure;

    subplot(3, 1, 1);                     % 3 rows, 1 column, subplot #i
    plot(data(1,:), LineWidth=2);
    title("Kalman Gain: X Direction");
    xlabel('Sample');
    ylabel('Value');
    grid on;
    subplot(3, 1, 2);                     % 3 rows, 1 column, subplot #i
    plot(data(2,:), LineWidth=2);
    title("Kalman Gain: Y Direction");
    xlabel('Sample');
    ylabel('Value');
    grid on;
    subplot(3, 1, 3);                     % 3 rows, 1 column, subplot #i
    plot(data(3,:), LineWidth=2);
    title("Kalman Gain: Z Direction");
    xlabel('Sample');
    ylabel('Value');
    grid on;

beautify_plot(gcf, 1);




    % plot(data(1,:), LineWidth=2); hold on; plot(data(3,:), LineWidth=2); hold on; plot(data(3,:), LineWidth=2);hold off;
    % title("Kalman Gain");
    % legend("x Direction", "y Direction", "z Direction")
    % xlabel('Sample');
    % ylabel('Value');
    % grid on;

end
