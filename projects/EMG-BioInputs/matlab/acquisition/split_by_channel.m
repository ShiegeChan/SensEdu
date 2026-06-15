function split_data = split_by_channel(data, ch_num)
%SPLIT_BY_CHANNEL  Unpacks a raw EMG stream into per-channel columns.
%   The ADC samples arrive interleaved [s0 s1 s2 s3 s0 s1 ...]; this reshapes
%   them into a (samples x ch_num) matrix, one column per channel.
    data = reshape(data, ch_num, []);
    split_data = data.';
end
