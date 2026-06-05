figure(101),
% for k = 1:ITERATIONS
% 
% 
% scatter(k,dist_matrix(1,k), 'ro', 'LineWidth', 1); 
% scatter(k,dist_matrix(2,k), 'bo', 'LineWidth', 1); 
% scatter(k,dist_matrix(3,k), 'mo', 'LineWidth', 1); 
%         hold on;
% 
%     %  for i = 4:6
%     %     scatter(k,dist_matrix(i,k), 'bo','LineWidth', 1); 
%     %     hold on;
%     % end
% 
% hold on,
% for i = 1:1
%     plot(k,y_vec(i,k), 'k*','LineWidth', 1); 
% end
% end
% 
% legend('m_1d_1', 'm_1d_2','m_1d_3',...
%     'D_1')

%%
first_peak = dist_matrix(1,:);
second_peak = dist_matrix(2,:);
third_peak = dist_matrix(3,:);
used_D = y_vec(4,:);
figure,
plot(first_peak, 'ro','LineStyle','none'); hold on;
plot(second_peak, 'bo','LineStyle','none'); hold on;
plot(third_peak, 'go','LineStyle','none'); hold on;
plot(used_D, 'k-','LineWidth',1.5); hold on;