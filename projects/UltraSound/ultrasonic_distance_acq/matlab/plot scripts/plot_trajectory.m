function plot_trajectory(state_history, microphones, REAL_MEASUREMENTS, pos_true)
    figure;
    plot3(state_history(1, :), state_history(2, :), state_history(3, :), "b-o", "LineWidth", 1.5, 'MarkerFaceColor','red', "MarkerEdgeColor",'none');
    hold on;
    plot3(state_history(1, 1), state_history(2, 1), state_history(3, 1), 'kx', LineWidth=2); % First predicted point
    hold on;
    for i=1:size(microphones,1)
        hold on;
        m = microphones(i,:);
        plot3(m(1), m(2), m(3), 'ko', LineWidth=2); 
    end
    
    if REAL_MEASUREMENTS == false
        plot3(pos_true(1, :), pos_true(2, :), pos_true(3, :), LineWidth=2, LineStyle="--"); % First predicted point
        legend('Generated Trajectory', 'Starting Point', 'Reference Trajectory', 'Location', 'best');
    else
        legend('Generated Trajectory', 'Starting Point', 'Microphones', 'Location', 'best');
    end
    hold off;
    xlabel('[m]');
    title('Object Tracking with EKF');
    grid on;
    beautify_plot(gcf, 0);
end