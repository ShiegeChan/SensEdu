% taken from deepseek
% Save all open figures
figHandles = findobj('Type', 'figure'); % Get handles to all open figures
for i = 1:length(figHandles)
    figure(figHandles(i)); % Bring the figure to focus
    beautify_plot(gcf, 1); % Apply beautification
    exportgraphics(gcf, sprintf('figure_%d.pdf', i), 'ContentType', 'vector'); % Save as PDF
end