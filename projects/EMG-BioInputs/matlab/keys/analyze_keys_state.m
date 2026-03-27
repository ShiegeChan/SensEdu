function keys_actions = analyze_keys_state(data, press_thresholds, release_thresholds)
    released_states = any(data < release_thresholds, 2)';
    pressed_states = any(data > press_thresholds, 2)';
    
    if any(released_states & pressed_states)
        released_states = released_states & ~pressed_states;
    end

    keys_actions = [released_states; pressed_states];
end