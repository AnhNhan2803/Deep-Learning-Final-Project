import pandas as pd
import matplotlib.pyplot as plt
import os
import sys
import numpy as np
from utils.dsp import filterSignal, rotate, jitter

# Load the data
data_path = "./data/training/SmartFall_Training.csv"
df = pd.read_csv(data_path)

window_size = 200
fall_event_size = 25

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python extractFallEvent.py <raw|filtered|noise|rotate>")
        sys.exit(1)

    arg = sys.argv[1]

    # More options so that we can have different way to visuallize data
    if arg == 'raw':
        pass
    elif arg == 'filtered':
        fs = 31.25 # config the sampling rate of IMU 31.25Hz
        filterOrder = 2
        fcutoffHigh = 15 # 15Hz to remove the high frequency noise
        fcutoffLow = 1 # 1Hz Low cut off frequency to remove the gravity
    elif arg == 'noise':
        pass
    elif arg == 'rotate':
        pass
    else:
        print(f"Unknown argument: {arg}")
        sys.exit(1)

    # Create a directory to save the figures if it doesn't already exist
    if not os.path.exists(f'./figures/accelerometet_fall_events_{arg}'):
        os.makedirs(f'./figures/accelerometet_fall_events_{arg}')

    # Find start of fall events
    fall_starts = np.where((df['outcome'].shift(-fall_event_size) == 0) & (df['outcome'] == 1) & (df['outcome'].shift(1) == 0))[0]

    # For each fall event
    for i, fall_event_start in enumerate(fall_starts):
        # Define the start and end of the window
        start = fall_event_start - window_size // 2
        end = start + window_size

        # Ensure window is within data boundaries
        if start < 0:
            start = 0
            end = start + window_size
        if end > len(df):
            end = len(df)
            start = end - window_size

        # Extract the window data
        window_data = df.iloc[start:end]

        if arg == 'filtered':
            data_x = filterSignal(window_data['ms_accelerometer_x'], 'lowpass', fcutoffHigh, fs, filterOrder)
            data_y = filterSignal(window_data['ms_accelerometer_y'], 'lowpass', fcutoffHigh, fs, filterOrder)
            data_z = filterSignal(window_data['ms_accelerometer_z'], 'lowpass', fcutoffHigh, fs, filterOrder)
            data_x = filterSignal(data_x, 'highpass', fcutoffLow, fs, filterOrder)
            data_y = filterSignal(data_y, 'highpass', fcutoffLow, fs, filterOrder)
            data_z = filterSignal(data_z, 'highpass', fcutoffLow, fs, filterOrder)
        elif arg == 'noise':
            data_x = jitter(window_data['ms_accelerometer_x'])
            data_y = jitter(window_data['ms_accelerometer_y'])
            data_z = jitter(window_data['ms_accelerometer_z'])

        elif arg == 'rotate':
            accel_data = window_data[['ms_accelerometer_x', 'ms_accelerometer_y', 'ms_accelerometer_z']].values
            # Rotate the data
            rotated_data = rotate(accel_data)
            # Replace the original data with the rotated data
            data_x = rotated_data[:, 0]
            data_y = rotated_data[:, 1]
            data_z = rotated_data[:, 2]

        # Plot the window data
        plt.figure(figsize=(12,8))
        plt.plot(data_x, label='Accelerometer in X axis')
        plt.plot(data_y, label='Accelerometer in Y axis')
        plt.plot(data_z, label='Accelerometer in Z axis')

        plt.ylabel('Accelerometer - m/s^2')
        plt.xlabel('Sample index')
        # Add title and legend to the figure
        plt.title(f'Fall event {i+1}')
        plt.legend()

        # Save the figure
        plt.savefig(f'./figures/accelerometet_fall_events_{arg}/fall_event_{i+1}.png')
        plt.close()
