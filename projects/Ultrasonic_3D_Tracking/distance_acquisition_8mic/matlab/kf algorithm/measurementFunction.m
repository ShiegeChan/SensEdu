function y = measurementFunction(x, microphones)
% Predicted distance to each microphone for the state x.
%
% The full acoustic path is speaker -> object -> microphone, but the firmware
% already halves the time of flight, so the model is halved to match.
    pos = x(1:3);
    d_obj = norm(pos);
    y = zeros(1, size(microphones, 1)); 
    for i = 1:length(y)
        y(i) = d_obj + norm(pos-microphones(i,:)');
    end
    y = (y./2)';
end