function H = jacobianMeasurement(x, microphones)
    pos = x(1:3);
    % distance from the speaker to the object
    dist_speaker_object = norm(pos);

    % Derivatives of the distance from the speaker
    ddist_speaker_object = pos / dist_speaker_object;
    
    H_pos = zeros(size(microphones, 1), 3);
    H_vel = zeros(size(microphones, 1), 3);

    for i = 1:size(microphones, 1)
        dist_obj_mic = norm(pos - microphones(i, :)');
        dd_obj_mic = (pos - microphones(i, :)') / dist_obj_mic;
        dd_dx = ddist_speaker_object + dd_obj_mic;
        H_pos(i, :) = dd_dx';
    end
    
    H = [H_pos, H_vel];
end