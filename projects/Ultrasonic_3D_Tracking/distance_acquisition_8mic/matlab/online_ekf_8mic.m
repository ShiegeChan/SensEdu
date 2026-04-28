% ONLINE EXTENDED KALMAN FILTER IMPLEMENTATION 
clear all
clc
close all
addpath("plot scripts\", "kf algorithm\");

%% Arduino Measurements
% Serial port configuration 
ARDUINO_PORT = 'COM10';
ARDUINO_BAUDRATE = 115200;
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate 

ITERATIONS = 350;
MIC_NUM = 8;
PEAKS_NUM = 2;
DETECTION_NUM = MIC_NUM*PEAKS_NUM;
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

microphones = [m1; m2; m3; m4; m8; m5; m6; m7];

% speaker 2: add offset to the microphone positions
if SPEAKER == 2
    for i = 1:MIC_NUM
        microphones(i,:) = microphones(i,:) + [0.035, 0.0, 0.0];
    end
end

%% EKF configuration
% initial position and velocity estimation [x; y; z; vx; vy; vz]
pos_init_estimate = [0.1; 0.1; 0.4]; 
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

% Other filter initializations
state_history = NaN(6, ITERATIONS);
err_vec = zeros(size(microphones,1), ITERATIONS);
K_vec = zeros(3, ITERATIONS);
y_vec = err_vec;
K_hist = zeros(6, ITERATIONS);
P_hist = zeros(6, 6, ITERATIONS);


%% Figures and plots
% % If you want the 3D plot:
% figure; 
% hold on;
% estimate_plot = plot3([pos_init_estimate(1)], [pos_init_estimate(2)], [pos_init_estimate(3)], "LineWidth", 2, "DisplayName", "Kalman Estimate", "Marker", "o");
% xlabel("Position x[m]"); ylabel("Position y[m]"); zlabel("Position z[m]");
% title("Object Tracking");
% grid on;
% xlim([-0.5,0.5]);
% ylim([-0.5,0.5]);
% zlim([0,2]);
% % axis([-0.3 0.3 -0.3 0.3 0.2 1.2]);
% view(3); % Ensure 3D perspective

% For a live plot of the cartesian coordinates
figure;
hold on;
hx = scatter(NaN, NaN, 30, 'ro'); % X
hx.XData = [];
hx.YData = [];
hy = scatter(NaN, NaN, 30, 'bo'); % Y
hy.XData = [];
hy.YData = [];
hz = scatter(NaN, NaN, 30, 'ko'); % Z
hz.XData = [];
hz.YData = [];
grid on;
xlim([0, ITERATIONS]); 
ylim([-1, 2.5]); 
legend('X','Y','Z')
hold on;
    

%% Filter Loop
pause(3) % The object needs to be already within the range in order for the current
% version to work. If the object is not immediately recognized, the
% estimations will be wrong
tic
t_prev = 0; 
for k = 1:ITERATIONS

    % Process distance data
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(k) = toc;
    t_current = toc; 
    dtau = t_current - t_prev;
    distances(:,k) = read_distance_data(arduino, DETECTION_NUM);
    
    if k == 1
        y = [distances(1:PEAKS_NUM:DETECTION_NUM,k)]; % initially take the 1st peak
        prev_best = y; % it's the best for now
    else       
        t_prev = t_current; 

        thr_peaks = 0.08; % we assume the target will not move more than this value between steps
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
                    y(m) = prev_best(1); % If none of the new peaks is good, stick to the previous one            
               end
            end
        end
    end    
    y_vec(:, k) = y; % just to save the measurements we input to the EKF


    % EKF prediction
    x_hat_prior = stateTransitionFunction(x_hat, dtau);
    F = jacobianStateTransition(x_hat, dtau);
    H = jacobianMeasurement(x_hat, microphones);
    % This model assumes generic dynamics:
    Q = dtau * [sigma_q^2 * I, (sigma_q^2/2)*dtau*I; 
               (sigma_q^2/2)*dtau*I, (sigma_q^2/3)*dtau^2*I];
    P_prior = F * P * F' + Q;

    % EKF measurement update
    err = (y - measurementFunction(x_hat, microphones));
    err_vec(:, k) = err; % store innovation residual
    S = H * P_prior * H' + R;
    K = P_prior * H' * S^(-1);
    K_vec(:, k) = K(1:3, 1); % store kalman gain values
    x_hat = x_hat_prior + K * err;

    % Joseph form of the covariance matrix has more numerical stability
    P = (eye(size(P)) - K * H) * P_prior*(eye(size(P)) - K * H)' + K*R*K'; 

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

    % for the 3d plot
    % estimate_plot.XData = state_history(1, 1:k);
    % estimate_plot.YData = state_history(2, 1:k);
    % estimate_plot.ZData = state_history(3, 1:k);
  
end


%% Check on the first microphone peaks
% Only valid for 3 peaks:
% first_peak = distances(1,:);
% second_peak = distances(2,:);
% third_peak = distances(3,:);
% used_D = y_vec(1,:);
% figure,
% plot(first_peak, 'ro','LineStyle','none'); hold on;
% plot(second_peak, 'bo','LineStyle','none'); hold on;
% plot(third_peak, 'go','LineStyle','none'); hold on;
% plot(used_D, 'k-','LineWidth',1.5); hold on;