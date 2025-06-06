% take from deepseek
function beautify_plot(figHandle, box)
    % Apply consistent beautification settings to all axes in a figure
    % Input:
    %   figHandle: Handle to the figure

    % Get all axes in the figure
    axHandles = findobj(figHandle, 'Type', 'axes');

    % Loop through each axes and apply settings
    for ax = axHandles'
        % Skip colorbars
        if isa(ax, 'matlab.graphics.axis.Axes') % Check if it's a regular axes object
            % Set axes properties
            set(ax, 'FontSize', 12); % Font size for axes
            set(ax, 'LineWidth', 1.2); % Axes line width
            % Check if the plot is 3D
            if box == 0
                set(ax, 'Box', 'off'); % Enable axes box
            else
                set(ax, 'Box', 'on'); % Enable axes box
            end
            set(ax, 'TickLabelInterpreter', 'latex'); % Use LaTeX for tick labels

            % Set labels and title to use LaTeX
            if ~isempty(ax.XLabel.String)
                set(ax.XLabel, 'Interpreter', 'latex', 'FontSize', 14);
            end
            if ~isempty(ax.YLabel.String)
                set(ax.YLabel, 'Interpreter', 'latex', 'FontSize', 14);
            end
            if ~isempty(ax.ZLabel.String)
                set(ax.ZLabel, 'Interpreter', 'latex', 'FontSize', 14);
            end
            if ~isempty(ax.Title.String)
                set(ax.Title, 'Interpreter', 'latex', 'FontSize', 15);
            end

            % Set legend properties
            if ~isempty(ax.Legend)
                set(ax.Legend, 'Interpreter', 'latex', 'FontSize', 12);
            end

            % Set grid
            grid(ax, 'on');
        end
    end

    % Set figure properties
    set(figHandle, 'Color', 'w'); % White background
    set(figHandle, 'Position', [100, 100, 800, 600]); % Set figure size
end