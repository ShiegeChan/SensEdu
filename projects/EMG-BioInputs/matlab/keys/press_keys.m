function keys_state = press_keys(robot, keys, keys_state, keys_press)
    to_press = find(keys_state == 0 & keys_press == 1);
    for i = to_press
        robot.keyPress(keys(i));
        keys_state(i) = 1;
    end
end