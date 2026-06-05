function msg = decode_fsk_message(wave, preamble, f, fs, hop, samples_per_bit, enable_plot)
    % Convolution Threshold
    % Chosen experimentally and may be required to change based on your
    % specific test conditions.
    conv_th = 1e10;

    % Goertzel Coefficient (must be integers)
    k = (f/fs) * samples_per_bit + 1;
    if (mod(k,2) ~= 0)
        warning("Goertzel Coefficient k is not integer. Unexpected behaviour; check the settings.")
    end
    
    % 1. Apply Goertzel to calculate each bit frequency energy.
    %    Whole dataset is processed multiple times with N/HOP offsets for further frame correction.
    [energy_diff, energy_x_labels] = run_goertzel(wave, hop, samples_per_bit, k);
    
    % 2. Find best preamble position and frame offset
    [best_hop, preamble_pos, best_conv] = analyze_preamble(energy_diff, preamble);
    best_energy_diff = energy_diff(best_hop, :);

    % 3. Stop if convolution result is too low
    if (best_conv < conv_th)
        msg = "";
        return;
    end
    
    if enable_plot == true
        % 3. Plot best convolution with preamble
        plot_conv(best_energy_diff, preamble, preamble_pos);
        
        % 4. Visualize Goertzel decisions with frame markers
        plot_goertzel(wave, best_energy_diff, energy_x_labels(best_hop, :), samples_per_bit);
    end

    % 5. Decode bitstream to ASCII message
    bits = energy2bits(best_energy_diff);
    msg = decode_bitstream(bits, preamble, preamble_pos);
end

function [energy_diff, x_labels] = run_goertzel(data, hop, N, k)
    bit_num = floor((length(data) - N)/N);
    hop_num = N/hop;

    energy_diff = zeros(hop_num, bit_num);
    x_labels = zeros(hop_num, bit_num);

    for j = 1:hop_num
        for i = 1:bit_num
            idx = (j-1)*hop + (i-1)*N + 1;
            segment = data(idx : (idx + N - 1));
    
            dtft1 = abs(goertzel(segment, k(1)))^2;
            dtft2 = abs(goertzel(segment, k(2)))^2;
        
            energy_diff(j, i) = dtft2 - dtft1;

            x_labels(j, i) = idx + N/2;
        end
    end
end

function [best_hop, best_preamble_pos, best_conv] = analyze_preamble(energy_diff, preamble)
    hop_num = size(energy_diff, 1);
    correlations = zeros(1, hop_num);
    preamble_pos = zeros(1, hop_num);

    for i = 1:length(correlations)
        data = energy_diff(i, :);
        c = abs(conv(data, fliplr((preamble * 2) - 1)));
        [correlations(i), idx] = max(c);
        preamble_pos(i) = idx - length(preamble) + 1;
    end

    [best_conv, best_hop] = max(correlations);
    best_preamble_pos = preamble_pos(best_hop);
end

function bitstream = energy2bits(energy)
    bitstream = energy > 0;
end

function msg = decode_bitstream(bits, preamble, preamble_pos)

    % i is the first sample of the payload
    i = preamble_pos + length(preamble);
    msg = "";

    while (i + 7 <= length(bits))
        ascii = bits(i:(i + 7));
        ascii_val = bit2int(ascii', 8);
                
        if (ascii_val == 0)
            break;
        end

        msg = msg + char(ascii_val);
        i = i + 8;
    end
end

function plot_conv(energy_diff, preamble, preamble_pos)
    c = conv(energy_diff, fliplr(preamble*2 - 1));
    figure(2);
    title("Preamble Convolution");
    plot(c);
    hold on;
    stem(energy_diff);
    stem(preamble_pos, max(energy_diff), 'g', 'LineWidth', 1.5);
    legend(["Convolution", "Goertzel Energy Difference", "Detected Preamble"]);
    xlabel("Bit Index");
    ylabel("Energy / Correlation");
    hold off;
end

function plot_goertzel(original_data, energy_array, energy_x_labels, N)
    figure(3);
    title("Goertzel Decision Frames")

    wave = original_data ./ max(abs(original_data));
    wave = wave - mean(wave);
    plot(wave, 'LineWidth', 1);
    hold on;

    energy_norm = energy_array ./ max(abs(energy_array));
    stem(energy_x_labels, energy_norm, 'LineWidth', 1.5);

    symbol_starts  = energy_x_labels - N/2;
    stem(symbol_starts, 0.5.*ones(1, length(symbol_starts)), '--', 'LineWidth', 0.5);
    
    legend(["Normalized Wave", "Frame Centers", "Frame Boundaries"]);
    xlabel("Sample Index");
    ylabel("Normalized Amplitude");
    hold off;
end