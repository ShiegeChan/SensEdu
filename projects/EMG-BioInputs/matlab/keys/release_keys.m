function keys_state = release_keys(robot, keys, keys_state, keys_release)
    to_release = find(keys_state == 1 & keys_release == 1);
    for i = to_release
        robot.mouseRelease(keys(i));
        keys_state(i) = 0;
    end
end