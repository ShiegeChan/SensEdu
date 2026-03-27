function filtered_data = apply_filter(data, fs, f0, f1, taps, delay)
    filtered_data = filter(b, 1, data);
    filtered_data = filtered_data((delay + 1):end);
end