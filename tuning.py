import os
import pandas as pd
from absl import app
from absl import flags
import numpy as np
from tensorflow.python.platform import resource_loader
from utils.fall_detection_algorithms import fallDetectionHyperModel
from tensorflow.keras.utils import to_categorical
from sklearn.model_selection import train_test_split
from utils.dsp import filterSignal, increase_fall_samples, rotate
from tensorflow.keras.callbacks import EarlyStopping

_USE_TFLITE_INTERPRETER = flags.DEFINE_bool(
    'use_tflite',
    True,
    'Inference with the TF Lite interpreter instead of the TFLM interpreter',
)

_PREFIX_PATH = resource_loader.get_path_to_datafile('')


def main(_):
    # Feed a window of 40 samples each time instead of 35 samples like in the paper
    windowSize = 40
    
    # If in 40 samples, 25 samples are marked as fall, then the whole window will be mared as fall
    fallThreshold = 25
    
    # Stride 2 samples each window
    strides = 5

    # Only consider a window as fall event if 25 samples marked as 1 lie in the boundary of [centerBoundary: windowSize - centerBoundary]
    centerBoundary = 0

    X = []
    y = []

    # Loop through the directory /data in bazel cache folder
    dataDir = os.path.expanduser(os.path.join(_PREFIX_PATH, 'data/training'))
    for fileName in os.listdir(dataDir):
        filePath = os.path.join(dataDir, fileName)
        if os.path.isfile(filePath):
            _data = pd.read_csv(os.path.join(_PREFIX_PATH, f'data/training/{fileName}')).to_numpy()
            X.extend(_data[:,:3].tolist())

            if 'Raw_Data_90ADL.csv' in fileName:
                # Raw_Data_90ADL.csv only has the ADL data without actual falling, then set ADL label for the whole dataset
                y.extend([[0.0] for i in range(len(_data[:,3:]))])    
            else:
                y.extend(_data[:,3:].tolist())   

    # Extract training and validation dataset to 40 samples chunk of data
    X = np.array([X[i:i + windowSize] for i in range(0, len(X) - windowSize + 1, strides)])
    # Since the dataset provided by Texas State University only account 25 fall samples as a set of samples around the peak
    # of the fall, then we can centrelized these 25 samples within the range of [4, windowSize - 4]
    y = np.array([1 if (np.sum(y[i + centerBoundary:i + windowSize - centerBoundary]) >= fallThreshold) else 0 for i in range(0, len(y) - windowSize + 1, strides)])
    y = to_categorical(y)

    # Split to training and testing set with 4/1 ratio
    X_train, X_val, y_train, y_val = train_test_split(X, y, test_size=0.2, shuffle=True)


    # Since I have trained this model in the unbalanced datasets, when the number of fall events are only 8%
    # Then I need to do several things to tackle this problem, I have done some data augmentation as above:
    # 1. Adding the resampling method (artificially increase the number of fall events by adding noise to existing fall events)
    # 2. Do the rotation for all 3 axes of the fall events to gurantee that the fall events can be detected
    # in all 3 axes (for example, we can fall to the left, fall the right, front and behind)
    # ** I only applied data augmentation on the training set
    X_train, y_train = increase_fall_samples(X_train, y_train)
    X_train_rotated = rotate(X_train)
    # Concat the normalized data with curren data
    X_train_augmented = np.concatenate([X_train, X_train_rotated])
    y_train_augmented = np.concatenate([y_train, y_train])

    fs = 31.25 # config the sampling rate of IMU 31.25Hz
    filterOrder = 2
    fcutoffHigh = 15 # 15Hz to remove the high frequency noise
    fcutoffLow = 1 # 1Hz Low cut off frequency to remove the gravity

    # Filter out the signal
    X_train_augmented = filterSignal(X_train_augmented, 'lowpass', fcutoffHigh, fs, filterOrder)
    X_train_augmented = filterSignal(X_train_augmented, 'highpass', fcutoffLow, fs, filterOrder)

    # Start running the model tunning here
    from kerastuner.tuners import RandomSearch

    # Define the hypermodel
    hypermodel = fallDetectionHyperModel(input_shape=(windowSize, 3, 1), num_classes=2)

    # Define the tuner
    tuner = RandomSearch(
        hypermodel,
        objective='val_accuracy',
        max_trials=20,
        directory='output',
        project_name='Fall_Detection_Hyperparameter_Tuning'
    )

    # Except for the 2 data augmentation methods at the beginning, I also added a very high penalty on 
    # misclassification of positive samples with class_weight below, I think that this one will be helpful
    # with the imbalanced dataset although we have the data augmentation above. I applied class_weight = 
    # {0: 1., 1: 10.}, whic assigns a weight of 1 to class 0 and a weight of 10 to class 1. This means
    # the model will consider an error on a class 1 example to be 10 times as important as an error on a 
    # class 0 example since class 1 is closer to the output compared to class 0.
    class_weight = {0: 1., 1: 10.}

    
    # I also add the Early Stopping during training so we do not waste time for more training if the result
    # does not improve after 5 consecutive epochs
    early_stopping = EarlyStopping(
        monitor='val_loss',  # Monitor validation loss
        patience=5,  # Number of epochs with no improvement after which training will be stopped
        restore_best_weights=True  # Whether to restore model weights from the epoch with the best value of the monitored quantity
    )

    # Start the search for the best hyperparameters
    tuner.search(
        x=X_train_augmented, 
        y=y_train_augmented, 
        epochs=20, 
        validation_data=(X_val, y_val),
        class_weight=class_weight,
        callbacks=[early_stopping]
    )

    # Get the optimal hyperparameters
    best_hps = tuner.get_best_hyperparameters(num_trials = 1)[0]

    print(best_hps)

if __name__ == '__main__':
  app.run(main)

