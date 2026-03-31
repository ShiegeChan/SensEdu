% ONLINE EXTENDED KALMAN FILTER IMPLEMENTATION 
clear all
clc
close all
addpath("plot scripts\");

%% Arduino Measurements
% Serial port configuration 
ARDUINO_PORT = 'COM10';
ARDUINO_BAUDRATE = 115200;
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate 

ITERATIONS = 500;
MIC_NUM = 8;
DETECTION_NUM = 8*3;
PEAKS_NUM = 3;
mic_name = {"MIC 1", "MIC 2","MIC 3", "MIC 4", "MIC 8", "MIC 6", "MIC 5", "MIC 7"};
DATA_LENGTH = 2048;
distances = zeros(DETECTION_NUM,ITERATIONS); 
time_axis = zeros(1, ITERATIONS); 

SPEAKER = 1; % 1 - speaker1 (ch1), 2 - speaker2(ch2)
m1 = [-0.09, 0.09, 0.0];
m2 = [-0.09, 0.0, 0.0];
m3 = [-0.09, -0.09, 0.0];
m4 = [0.0, -0.09, 0.0];
m5 = [0.09, -0.09, 0.0];
m6 = [0.09, 0.0, 0.0];
m7 = [0.09, 0.09, 0.0];
m8 = [0.00, 0.09, 0.0];
% microphones_no_off = [m1; m2; m3; m4; m8; m6; m5; m7];
microphones = [m1; m2; m3; m4; m8; m6; m5; m7];

% speaker 2: add offset to the microphone positions
if SPEAKER == 2
    for i = 1:MIC_NUM
        microphones(i,:) = microphones(i,:) + [0.035, 0.0, 0.0];
    end
end

%% EKF configuration
% initial position and velocity estimation [x; y; z; vx; vy; vz]
pos_init_estimate = [0.0; 0.0; 0.65]; 
vel_init_estimate = [1e-5; 1e-5; -0.01]; 
x_hat = [pos_init_estimate; vel_init_estimate];

% prediction error covariance matrix
sigma_p = 0.05; 
P =  eye(6) * (sigma_p^2); 

% process noise covariance matrix (related to our model which is not PERFECT!)
I = eye(3); 
sigma_q = 0.02; 

% measurement noise covariance matrix
sigma_r = 0.01;
R = diag(ones(1, size(microphones,1))*sigma_r^2);

state_history = NaN(6, ITERATIONS);
err_vec = zeros(size(microphones,1), ITERATIONS);
K_vec = zeros(3, ITERATIONS);
y_vec = err_vec;
K_hist = zeros(6, ITERATIONS);
P_hist = zeros(6, 6, ITERATIONS);


%% Figures and plots
% figure; 
% hold on;
% estimate_plot = plot3([pos_init_estimate(1)], [pos_init_estimate(2)], [pos_init_estimate(3)], "LineWidth", 2, "DisplayName", "Kalman Estimate", "Marker", "o");
% xlabel("Position x[m]"); ylabel("Position y[m]"); zlabel("Position z[m]");
% title("Object Tracking");
% grid on;
% xlim([-0.5,0.5]);
% ylim([-0.5,0.5]);
% zlim([0,2.5]);
% % axis([-0.3 0.3 -0.3 0.3 0.2 1.2]);
% view(3); % Ensure 3D perspective

% figure;
% hold on;
% h = scatter(NaN, NaN, 40, 'rx'); % Create an empty scatter handle
% h.XData = [];
% h.YData = [];
% xlim([0, ITERATIONS]);  % Set your fixed X-axis range (e.g., 0 to 100 steps)
% ylim([0, 2.5]); % Set your expected Y-axis range

figure;
hold on;
hx = scatter(NaN, NaN, 30, 'rx'); % X
hx.XData = [];
hx.YData = [];
hy = scatter(NaN, NaN, 30, 'bo'); % Y
hy.XData = [];
hy.YData = [];
hz = scatter(NaN, NaN, 30, 'k*'); % Z
hz.XData = [];
hz.YData = [];
grid on;
xlim([0, ITERATIONS]); 
ylim([-1, 2.5]); 
legend('X','Y','Z')
hold on;
    

%% Filter Loop
pause(3) % The object needs to be already within the range in order for the current
% version to work -> FIX later
tic
t_prev = 0; 
for k = 1:ITERATIONS

    % Process distance data
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(k) = toc;
    t_current = toc; 
    dtau = t_current - t_prev
    distances(:,k) = read_distance_data(arduino, DETECTION_NUM);
    
    if k == 1
        % dtau = 0.04;
        y = [distances(1:3:24,k)]; % initially take the 1st peak
        prev_best = y; % it's the best for now
    else       
        t_prev = t_current; 

        thr_peaks = 0.07; % we assume the target will not move more than this value between steps
        for m = 1:MIC_NUM
            for j = 1:PEAKS_NUM
                % We want to check which among the peaks is the best one,
                % i.e., the one closer to the previous estimate. This will
                % be sent to the filter as measurement (y).>
               if (abs(distances(PEAKS_NUM*(m-1)+j,k) - y_vec(m,k-1)) <= thr_peaks)
                   y(m) = distances(PEAKS_NUM*(m-1)+j,k);
                   prev_best = y(m);
                   break; % already done for the mic m
               else
                    y(m) = prev_best(1); % to be sure in case none of them work            
               end
            end
        end
    end    
    y_vec(:, k) = y; % just to save the measurements we input to the EKF
    %     new_y = y;
    %     new_x = k*ones(1,8);
    %     h.XData = [h.XData, new_x];
    %     h.YData = [h.YData, new_y'];
    % drawnow

    % EKF prediction
    x_hat_prior = stateTransitionFunction(x_hat, dtau);
    F = jacobianStateTransition(x_hat, dtau);
    H = jacobianMeasurement(x_hat, microphones);
    % Check Harald's Thesis for this model:
    Q = dtau * [sigma_q^2 * I, (sigma_q^2/2)*dtau*I; 
               (sigma_q^2/2)*dtau*I, (sigma_q^2/3)*dtau^2*I];
    % Q = sigma_q^2*eye(6);
    P_prior = F * P * F' + Q;

    % EKF measurement update
    err = (y - measurementFunction(x_hat, microphones));
    err_vec(:, k) = err; % store innovation residual
    S = H * P_prior * H' + R;
    K = P_prior * H' * S^(-1);
    K_vec(:, k) = K(1:3, 1); % store kalman gain values
    x_hat = x_hat_prior + K * err;

    % P = (eye(size(P)) - K * H) * P_prior;
    P = (eye(size(P)) - K * H) * P_prior*(eye(size(P)) - K * H)' + K*R*K'; % Joseph Form


    % storing
    state_history(:, k) = [x_hat(1:3);x_hat(4:6)];


    % Plotting
    new_y = x_hat(1:3);
    new_x = k;
    hx.XData = [hx.XData, new_x];
    hx.YData = [hx.YData, new_y(1)];
    hy.XData = [hy.XData, new_x];
    hy.YData = [hy.YData, new_y(2)];
    hz.XData = [hz.XData, new_x];
    hz.YData = [hz.YData, new_y(3)];
    drawnow

    % estimate_plot.XData = state_history(1, 1:k);
    % estimate_plot.YData = state_history(2, 1:k);
    % estimate_plot.ZData = state_history(3, 1:k);
  
end



figure(100),
for i = 1:24
    scatter(k,distances(i,k), 'o'); 
    hold on;
end

hold on,
for i = 1:8
    plot(y_vec(i,:), '*'); 
 
end