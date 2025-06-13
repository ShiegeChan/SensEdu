function y = measurementFunction(x, microphones)
    pos = x(1:3);
    d_obj = norm(pos);
    y = zeros(1, size(microphones, 1)); 
    for i = 1:length(y)
        y(i) = d_obj + norm(pos-microphones(i,:)');
    end
    y = (y./2)';
end