function x_next = stateTransitionFunction(x, dt)
    x_next = [x(1) + x(4) * dt; 
              x(2) + x(5) * dt;
              x(3) + x(6) * dt;
              x(4); x(5); x(6)];
end