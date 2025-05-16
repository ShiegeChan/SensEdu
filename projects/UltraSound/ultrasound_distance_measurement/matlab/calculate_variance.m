load("measurements_variance_1_14-Mar-2025_13-08-41.mat");

mic1 = dist_matrix(4, 301:399);
m1_var = sqrt(var(mic1))
plot(mic1)
