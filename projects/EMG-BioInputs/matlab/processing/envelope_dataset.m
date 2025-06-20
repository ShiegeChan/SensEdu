function enveloped_dataset = envelope_dataset(dataset, fs)
    envelope_cutoff = 20;  % LP filter in Hz
    [b_env, a_env] = butter(4, envelope_cutoff / (fs / 2), 'low');
    %[A, B, C, D] = butter(4, envelope_cutoff / (fs / 2), 'low');
    %sys = ss(A,B,C,D,1/fs);
    %step(sys)

    enveloped_dataset = zeros(size(dataset));
    for i = 1:size(dataset,1)
        enveloped_dataset(i,:)  = filtfilt(b_env, a_env, dataset(i,:));
    end
end