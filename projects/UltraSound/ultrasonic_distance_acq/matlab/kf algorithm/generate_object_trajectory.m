function [x_true] = generate_object_trajectory(t, trajectory_type, min_distance)
%   GENERATE_OBJECT_TRAJECTIORY Model of the object trajectory
%   Outputs decartes coordinates for the object in each time instance.
%   t - time vector
%   trajectory_type (helix, updown, random) - type of the trajectory
%   min_distance - specifies the minimum distancedetectable by the real 
%   system microphones in meters. 

% choices
if trajectory_type == "helix"
    radius = 0.02; % Radius of the helix (meters)
    num_coils = 4; % Number of coils
    total_height = 1.0; % Total height in z-direction (meters)
    speed = 0.04; % Speed in z-direction (meters/second)
    
    % Calculate pitch
    pitch = total_height / num_coils; % Vertical distance between coils (meters)
    t_total = total_height / speed;

    % Preallocate arrays for positions
    xo = zeros(1, length(t));
    yo = zeros(1, length(t));
    zo = zeros(1, length(t));
    
    % Generate the helix trajectory
    for i = 1:length(t)
        % Calculate the angular position
        theta = 2 * pi * num_coils * t(i) / t_total; % Angle in radians
    
        % Calculate the positions
        xo(i) = radius * cos(theta); % x-position
        yo(i) = radius * sin(theta); % y-position
        zo(i) = 0.3 + speed * t(i); % z-position (starts at 0.3 meters)
    end
elseif trajectory_type == "updown"
    xo = zeros(1, length(t))+0.6;
    yo = zeros(1, length(t))+0.3;
    zo = linspace(0.2, 0.5, length(t));
elseif trajectory_type == "horizontal"
    xo = linspace(-0.5, 0.8, length(t));
    yo = linspace(-0.5, 0.8, length(t));
    zo = zeros(1, length(t)) + 0.6;
elseif trajectory_type == "squared"
    [xo, yo, zo] = square_trajectory(t);
elseif trajectory_type == "random"
    N = length(t);
    dt = t(2) - t(1);
    v0 = [1e-6; 1e-6; 1e-5]; % initial velocity
    v = zeros(3, N+1);
    z = zeros(3, N+1);
    
    sigma = 0.2;
    z0 = [0; 0; min_distance];
    v(:,1) = v0;
    z(:,1) = z0;
    
    % Generate random increments and compute the Wiener process
    for i = 2:N+1
        alpha = sigma * sqrt(dt) * randn(3,1);
        beta = sigma * sqrt(dt^3) * randn(3,1);
        
        v(:, i) = v(:, i-1) + alpha;
        z(:,i) = z(:,i-1) + v(:,i)*dt + beta;
        if (z(3,i) < 0)
            z(3,i) = -z(3,i);
        elseif(z(3,i) < min_distance)
            z(3,i) = min_distance;
        elseif(z(3,i) > 2)
            z(3,i) = 2;
        end
    end

    xo = z(1, :); yo = z(2, :); zo = z(3, :);
end

% assign the values
x_true = [xo; yo; zo];

end

