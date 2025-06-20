function filtered_dataset = filter_dataset(dataset, taps)
    filtered_dataset = zeros(size(dataset));
    for i = 1:size(dataset,1)
        filtered_dataset(i,:) = filter(taps, 1, dataset(i,:));
    end
end