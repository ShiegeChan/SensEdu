function split_data = split_by_channel(data, ch_num)
%SPLIT_BY_CHANNEL  Unpack a raw EMG stream into per-channel columns.
%   The ADC samples arrive interleaved [c0 c1 c2 c3 c0 c1 ...]; this reshapes
%   them into a (samples x ch_num) matrix, one column per channel.
    data = reshape(data, ch_num, []);
    split_data = data.';
end
