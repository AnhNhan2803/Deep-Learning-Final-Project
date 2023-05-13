from scipy.signal import butter, sosfilt
import numpy as np

def filterSignal(data, filterType, cutoffFreq, fs, order, notchWidth=None):
    if filterType == 'lowpass':
        sos = butter(order, cutoffFreq, fs=fs, btype='low', analog=False, output='sos')
    elif filterType == 'highpass':
        sos = butter(order, cutoffFreq, fs=fs, btype='high', analog=False, output='sos')
    elif filterType == 'bandpass':
        sos = butter(order, cutoffFreq, fs=fs, btype='band', analog=False, output='sos')
    elif filterType == 'bandstop' or filterType == 'notch':
        if filterType == 'notch':
            if notchWidth is None:
                raise ValueError("Please provide notchWidth for the notch filter.")
            low = cutoffFreq - notchWidth / 2
            high = cutoffFreq + notchWidth / 2
            cutoffFreq = [low, high]
        sos = butter(order, cutoffFreq, fs=fs, btype='bandstop', analog=False, output='sos')
    else:
        raise ValueError("Invalid filter type. Choose from 'lowpass', 'highpass', 'bandpass', 'bandstop', or 'notch'.")
    
    # Putting data through the sosfilt (cascaded second-order filter) after config the filter 
    # characteristics, use the sosfilt to improve performance in run time, sosfilt is also 
    # compatible with the CMSIS-DSP library supported on ARM Microcontroller. As a result,
    # using this will be good in case we want to deploy the whole program to light weight MCU
    filtered_data = sosfilt(sos, data)
    return filtered_data

# Some function for data augmentation to solve the imbalanced dataset
def jitter(data, sigma=0.5):
    myNoise = np.random.normal(loc=0, scale=sigma, size=data.shape)
    return data + myNoise

def increase_fall_samples(X_train, y_train, target_ratio=0.4):
    # Convert y_train to 1D array
    y_train_1d = np.argmax(y_train, axis=1)
    
    # Get fall and non-fall data
    fall_data = X_train[y_train_1d == 1]
    non_fall_data = X_train[y_train_1d == 0]
    
    # Calculate how many new samples we need
    num_fall = len(fall_data)
    num_non_fall = len(non_fall_data)
    num_new_samples = int((target_ratio * num_non_fall - num_fall) / (1 - target_ratio))
    
    # Generate new samples
    indices = np.random.randint(0, num_fall, num_new_samples)
    new_samples = jitter(fall_data[indices])
    
    # Concatenate with the original data
    X_train_augmented = np.concatenate([new_samples, X_train])
    y_train_augmented = np.concatenate([np.tile([0, 1], (num_new_samples, 1)), y_train])  
    
    return X_train_augmented, y_train_augmented


def rotate(X, angle_range=(0, 360)):
    """Randomly rotates 3D vectors in the X array along a specified axis."""
    # divide data into three parts
    num_samples = X.shape[0]
    indices = np.random.permutation(num_samples)
    third = num_samples // 3

    for i, axis in enumerate(['x', 'y', 'z']):
        start_idx = i * third
        end_idx = (i + 1) * third if i < 2 else num_samples
        idxs = indices[start_idx:end_idx]

        theta = np.deg2rad(np.random.uniform(*angle_range))  # random rotation angle

        if axis == 'x':
            R = np.array([[1, 0, 0],
                          [0, np.cos(theta), -np.sin(theta)],
                          [0, np.sin(theta), np.cos(theta)]])
        elif axis == 'y':
            R = np.array([[np.cos(theta), 0, np.sin(theta)],
                          [0, 1, 0],
                          [-np.sin(theta), 0, np.cos(theta)]])
        else:  # 'z'
            R = np.array([[np.cos(theta), -np.sin(theta), 0],
                          [np.sin(theta), np.cos(theta), 0],
                          [0, 0, 1]])

        X[idxs] = np.dot(X[idxs], R)

    return X
