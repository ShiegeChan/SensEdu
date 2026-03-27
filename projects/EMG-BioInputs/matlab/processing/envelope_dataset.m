function enveloped_dataset = envelope_dataset(dataset, fs)
    
    envelope_cutoff = 50;  % LP filter in Hz
    [b_env, a_env] = butter(2, envelope_cutoff / (fs / 2), 'low');
    %[A, B, C, D] = butter(4, envelope_cutoff / (fs / 2), 'low');
    %sys = ss(A,B,C,D,1/fs);
    %step(sys)

    enveloped_dataset = zeros(size(dataset));
    for i = 1:size(dataset,1)
        enveloped_dataset(i,:)  = filtfilt(b_env, a_env, dataset(i,:));
    end
    
    
    % RMS
    %{
    window_size = round(0.05 * fs); % 50 ms moving window
    for i = 1:size(dataset, 1)
        enveloped_dataset(i, :) = sqrt(movmean(dataset(i, :).^2, window_size));
    end
    %}
end