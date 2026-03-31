function dataArray = csv_redout(filePath)
    %read_csv_data Convert exported Audio Precision CSV data to convenient for plotting format
    %inputs:
    %   file_path - csv file location
    %outputs:
    %   sets(measurement_set, sample, axis) - numerical data
    %   titles(measurement_set) - name of the measurement set
    %   labels(measurement_set, axis) - units of X and Y in this measurement set
    
    % T = readcell(file_path);
    % 
    % sets_start_idxs = 8;
    % sets_num = numel(sets_start_idxs);
    % 
    % % 4 rows are for title, labels, channels etc
    % set_size_row = sets_start_idxs(2) - sets_start_idxs(1) - 8;
    % 
    % sets = zeros(sets_num, set_size_row, set_size_column); % data container
    % for i = 1:sets_num
    %     set_cell = T(sets_start_idxs(i):(sets_start_idxs(i)+set_size_row-1),1:set_size_column);
    %     sets(i,:,:,:,:,:,:) = str2double(replace(string(set_cell),',','.'));
    % end
    % Open the file
    fid = fopen(filePath, 'r');
    
    % Read the first line
    firstLine = fgetl(fid);
    
    % Close the file
    fclose(fid);
    
    % Split the first line into a cell array using the delimiter ','
    dataParts = split(firstLine, ',');
    
    % Find the index of "Total Exported Frames"
    frameIndex = find(strcmp(dataParts, 'Total Exported Frames'));
    
    % Extract the value next to "Total Exported Frames"
    if ~isempty(frameIndex)
        totalExportedFrames = str2double(dataParts{frameIndex+1});
        fprintf('Total Exported Frames: %d\n', totalExportedFrames);
    else
        error('Total Exported Frames not found in the CSV file.');
    end

    
    % Define the range of rows to read (row 7 to totalExportedFrames + 6)
    startRow = 8; % Data starts at row 7
    endRow = startRow + totalExportedFrames - 1;
    
    % Use readtable to import the relevant part of the data
    opts = detectImportOptions(filePath); % Automatically detect file structure
    opts.DataLines = [startRow, endRow];  % Specify rows to read
    data = readtable(filePath, opts);
    
    %% Display or process the data

    % (Optional) Convert to an array for numerical processing
    dataArray = table2array(data);
end