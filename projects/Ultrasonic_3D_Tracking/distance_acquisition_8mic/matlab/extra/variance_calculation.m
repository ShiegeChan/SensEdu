sum1 = 0;
sum2 = 0;
std_dev_vec = zeros(4, 5);
var_vec = zeros(4, 5);
mu_vec = zeros(4, 5);
for i = 1:size(dist_matrix, 1)
    cnt = 1;
    if i == 3
        continue;
    end
    % choose mic
    for j = 1:100:size(dist_matrix, 2)
        % diff distances
        sig = dist_matrix(i, j:j+99);
        std_dev = std(sig);
        mu_vec(i, cnt) = mean(sig);
        std_dev_vec(i, cnt) = std_dev;
        var_sig = var(sig);
        var_vec(i, cnt) = var_sig;
        cnt = cnt+1;
    end
end


figure
plot( dist_matrix(1,:),LineWidth=2); hold on; 
plot( dist_matrix(2,:),LineWidth=2); hold on; 
plot( dist_matrix(3,:),LineWidth=2); hold on; 
plot( dist_matrix(4,:),LineWidth=2); hold off; 
legend("Mic 1", "Mic 2", "Mic 3", "Mic 4");

title("Microphone measurements for determining measurement noise variance")
xlabel("sample number k");
ylabel("distance [m]")

for k = 1:5
    distances(k) = mean(mu_vec(:, k));
end

beautify_plot(gcf, 1);
save_plot

%% how is it changing with distance

figure;
for mic = 1:4
    if mic == 1
        markers = 'square';
    elseif mic == 2
        markers = 'o';
    else
        markers = 'diamond';
    end

    if mic == 3
        continue;
    end
    plot(distances, std_dev_vec(mic,:), Marker=markers, LineWidth=2, MarkerSize=10); hold on;
     
    title('Measurement noise');
    ylabel('Standard Deviation [m]');    
    xlabel('Average distance measured [m]');
    
end
legend("Mic 1", "Mic 2", "Mic 4", Location="southeast");
beautify_plot(gcf, 1);
save_plot

%%

time = time_axis(1:99);

figure;
for mic = 1:4
    subplot(4, 1, mic); % Create subplot for each microphone
    data = dist_matrix(mic, 1:99);
    mu = mean(data); % Mean distance
    sigma = std(data); % Standard deviation (sqrt of variance)
    
    % Plot individual measurements
    plot(time, data, "o-", LineWidth=2);
    hold on;
    ylim([0.334 0.36])
    % Plot mean line
    plot([time(1), time(end)], [mu, mu], 'LineWidth', 1.5, 'Color', [0 0 0 0.4]);
    
    % Shaded region for ±1 standard deviation
    fill([time, fliplr(time)], ...
         [mu + sigma*ones(size(time)), fliplr(mu - sigma*ones(size(time)))], ...
         'r', 'FaceAlpha', 0.2, 'EdgeColor', 'none');
    
    title(['Microphone ', num2str(mic)]);
    ylabel('Distance [m]');
    if mic == 4
        xlabel('Time [s]');
    end
    legend('Measurements', 'Mean', '+/- $1\sigma$', 'Location', 'best');
    hold off;
end
beautify_plot(gcf, 1);
save_plot