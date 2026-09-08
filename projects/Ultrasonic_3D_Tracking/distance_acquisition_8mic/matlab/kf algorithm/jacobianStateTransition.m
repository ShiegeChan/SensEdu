function F = jacobianStateTransition(x, dt) 
% Jacobian of stateTransitionFunction. Constant for a constant-velocity model.
    F = [1 0 0 dt 0 0;
         0 1 0 0 dt 0;
         0 0 1 0 0 dt;
         0 0 0 1 0 0;
         0 0 0 0 1 0;
         0 0 0 0 0 1];
end