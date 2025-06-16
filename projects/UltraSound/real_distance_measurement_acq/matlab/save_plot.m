% taken from deepseek
% Save all open figures

function save_plot(plot_name)
figHandles = findobj('Type', 'figure'); % Get handles to all open figures
for i = 1:length(figHandles)
    figure(figHandles(i)); % Bring the figure to focus
    beautify_plot(gcf, 1); % Apply beautification
    exportgraphics(gcf, sprintf('%s.pdf', plot_name), 'ContentType', 'vector'); % Save as PDF
end