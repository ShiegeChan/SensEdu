%% plot_example.m
% don't forget to put the copy of this file into subfolder for the pathes to work properly
clear;
close all;
clc;

addpath 'kf algorithm'\ 'plot scripts'\

%% Consts
PATH = "dronehalle_measurements/ground_t_1.csv";

%% Optitrack
sets = csv_redout(PATH);

td = sets(:,2);
end_offset=1100;
begin_offset = 120;
step = 1;
%%
x_true = sets(begin_offset:step:end-end_offset, 6) - 0.856117;
z_true = sets(begin_offset:step:end-end_offset, 7) - 0.595332;
y_true = sets(begin_offset:step:end-end_offset, 8) - 0.334957;
td = td(begin_offset:step:end-end_offset);
plot3(x_true,y_true,z_true, "-o");
hold on;
plot3(x_true(1), y_true(1), z_true(1), LineWidth=2, Marker="o");
title("OptiTrack Measurements")
xlabel("x [m]"); ylabel("y [m]"); zlabel("z [m]");
grid on

m1 = [-0.09, 0.09, 0.0];
m2 = [-0.09, 0.0, 0.0];
m3 = [-0.09, -0.09, 0.0];
m4 = [0.0, -0.09, 0.0];
m5 = [0.09, -0.09, 0.0];
m6 = [0.09, 0.0, 0.0];
m7 = [0.09, 0.09, 0.0];
m8 = [0.00, 0.09, 0.0];
microphones_no_off = [m1; m2; m3; m4; m8; m6; m5; m7];
microphones = [m1; m2; m3; m4; m8; m6; m5; m7];

%% getting the measurements from the model using optitrack data
pos_true = [x_true, y_true, z_true]';
for i = 1:size(td, 1)
    d = get_measurements_fun(pos_true(:,i), microphones, 0.05);
    dist_matrix_ot(:,i) = d;
end

%% SensEdu

data_se = load("dronehalle_measurements/ground_t__1.mat");
dist_matrix_se = data_se.dist_matrix;
dist_matrix_se =  outlier_rejection(dist_matrix_se);
time_se = data_se.time_axis;

%% Plots
plot_measurements(dist_matrix_ot);
plot_measurements(dist_matrix_se);

%% Kalman filter for optitrack data 


