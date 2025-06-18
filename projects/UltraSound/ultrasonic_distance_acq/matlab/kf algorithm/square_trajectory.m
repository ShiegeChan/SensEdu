function [x, y, z] = square_trajectory(~)
    % Parameters
    dt = 0.15; % time step in seconds
    speed = 0.02; % speed in m/s (20 cm/s)
    side_length = 0.02; % length of the square side in meters
    num_points_per_side = round(side_length / (speed * dt)); % number of points per side
    height_steps = [0.21, 0.25]; % heights in meters
    time_per_side = num_points_per_side * dt; % time to traverse one side
    vertical_speed = 0.2; % vertical speed in m/s (20 cm/s)
    height_transition_time = abs(diff(height_steps)) / vertical_speed; % time to change height
    num_vertical_points = round(height_transition_time / dt); % number of points for vertical transition
    
    % Initialize arrays
    x = [];
    y = [];
    z = [];
    time = [];
    
    % Function to generate points for one side of the square
    generate_side = @(start, stop, num_points) linspace(start, stop, num_points);
    
    current_time = 0;
    
    % Generate the trajectory for each height step
    for i = 1:length(height_steps)
        h = height_steps(i);
        
        % Generate the four sides of the square
        x_side1 = generate_side(-0.1, 0.1, num_points_per_side);
        y_side1 = -0.1 * ones(1, num_points_per_side);
        z_side1 = h * ones(1, num_points_per_side);
        
        x_side2 = 0.1 * ones(1, num_points_per_side);
        y_side2 = generate_side(-0.1, 0.1, num_points_per_side);
        z_side2 = h * ones(1, num_points_per_side);
        
        x_side3 = generate_side(0.1, -0.1, num_points_per_side);
        y_side3 = 0.1 * ones(1, num_points_per_side);
        z_side3 = h * ones(1, num_points_per_side);
        
        x_side4 = -0.1 * ones(1, num_points_per_side);
        y_side4 = generate_side(0.1, -0.1, num_points_per_side);
        z_side4 = h * ones(1, num_points_per_side);
        
        % Concatenate the points for the full square
        x = [x, x_side1, x_side2, x_side3, x_side4];
        y = [y, y_side1, y_side2, y_side3, y_side4];
        z = [z, z_side1, z_side2, z_side3, z_side4];
        time_segment = current_time:dt:(current_time + 4*time_per_side - dt);
        time = [time, time_segment];
        
        % Update current time
        current_time = current_time + 4 * time_per_side;
        
        % If not the last height step, add vertical transition
        if i < length(height_steps)
            next_height = height_steps(i + 1);
            z_transition = generate_side(h, next_height, num_vertical_points);
            x_transition = repmat(x(end), 1, num_vertical_points);
            y_transition = repmat(y(end), 1, num_vertical_points);
            time_transition = current_time:dt:(current_time + height_transition_time - dt);
            
            x = [x, x_transition];
            y = [y, y_transition];
            z = [z, z_transition];
            time = [time, time_transition];
            
            % Update current time
            current_time = current_time + height_transition_time;
        end
    end
end

% % Plot the trajectory
% figure;
% plot3(x, y, z, 'b-', 'LineWidth', 2);
% hold on;
% scatter3(x, y, z, 'r');
% xlabel('X (m)');
% ylabel('Y (m)');
% zlabel('Z (m)');
% title('3D Drone Trajectory with Vertical Transitions');
% grid on;
% axis([-1.5 1.5 -1.5 1.5 0 1.5]);
% legend('Trajectory', 'Waypoints');
% hold off;
% 
% % Print trajectory details
% disp('Generated 3D Drone Trajectory:');
% disp(table(time(:), x(:), y(:), z(:), 'VariableNames', {'Time', 'X', 'Y', 'Z'}));