function [filtered_measurements] = outlier_rejection(measurement_data)
%OUTLIER_REJECTION Cleaning up the measurements
%   filtered_measurements - new measurement data after outlier rejection
%   measurement_data - input measurements to be examined

filtered_measurements = zeros(size(measurement_data));

for i = 1:size(measurement_data, 1)
    filtered_measurements(i,1) = measurement_data(i,1);
    for j = 2:size(measurement_data, 2)
        current_sample = measurement_data(i,j);
        previous_sample = measurement_data(i,j-1);
        if (abs(previous_sample - current_sample) >= 0.09)
            current_sample = previous_sample;
        end
        filtered_measurements(i,j) = current_sample;
        measurement_data(i,j) = current_sample;
    end
end
end