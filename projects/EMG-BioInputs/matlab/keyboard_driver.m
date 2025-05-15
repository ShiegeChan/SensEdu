%% keyboard_driver.m
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;

QUIT_FLAG = false;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate

%% Readings Loop
while(~QUIT_FLAG)
    QUIT_FLAG = true;
end

% set COM port back free
arduino = [];