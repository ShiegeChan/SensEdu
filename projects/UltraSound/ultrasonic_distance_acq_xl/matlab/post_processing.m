clear;
load("dist_kf.mat")
MIC_NUM = 6;
figure
for i = 1:MIC_NUM
    plot(dist_matrix(i, :), 'LineWidth', 2); hold on;
end
hold off;
