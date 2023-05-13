import os
import pandas as pd
from absl import app
from absl import flags
import numpy as np
from tensorflow.python.platform import resource_loader
from utils.fall_detection_algorithms import fallDetectionStandard, fallDetectionTflite
from tensorflow.keras.utils import to_categorical
from sklearn.model_selection import train_test_split
from utils.dsp import filterSignal, increase_fall_samples, rotate

_USE_TFLITE_INTERPRETER = flags.DEFINE_bool(
    'use_tflite',
    True,
    'Inference with the TF Lite interpreter instead of the TFLM interpreter',
)

_PREFIX_PATH = resource_loader.get_path_to_datafile('')


# Main function
def main(_):
    # Feed a window of 40 samples each time instead of 35 samples like in the paper
    windowSize = 40
    
    # If in 40 samples, 25 samples are marked as fall, then the whole window will be mared as fall
    fallThreshold = 25
    
    # Stride 2 samples each window
    strides = 2

    # Only consider a window as fall event if 25 samples marked as 1 lie in the boundary of [centerBoundary: windowSize - centerBoundary]
    centerBoundary = 4

    # Need to apply the absolute path since the bazel-bin can not recognize the relative path when running the script
    modelDir = '/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/model'
    modelName = 'fall_detection_model'

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
                # Raw_Data_90ADL.csv only has the ADL data without actual falling
                # Then set ADL label for the whole dataset
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

    # Do some data augmentation here
    X_train, y_train = increase_fall_samples(X_train, y_train)

    X_train_rotated = rotate(X_train)

    X_train_augmented = np.concatenate([X_train, X_train_rotated])
    y_train_augmented = np.concatenate([y_train, y_train]) 

    fs = 31.25 # config the sampling rate of IMU 50Hz
    filterOrder = 2
    fcutoffHigh = 15 # 15Hz
    fcutoffLow = 1 # 15Hz

    X_train_augmented = filterSignal(X_train_augmented, 'lowpass', fcutoffHigh, fs, filterOrder)
    X_train_augmented = filterSignal(X_train_augmented, 'highpass', fcutoffLow, fs, filterOrder)

    ###################################################################################
    tfStandardModel = fallDetectionStandard(windowSize=windowSize, kernelSize=3, dropOutRate=0.2, poolSize=2, verbose=False)

    print('Start the model training')
    # Fit the model with training data and do the validation based on the validation set
    history = tfStandardModel.train(trainData=(X_train_augmented, y_train_augmented), validationData=(X_val, y_val))
    # Plot the loss function, accuracy and ROC curve of the model and save to the path /figures/initial_model_validation/

    tfStandardModel.plot_metrics(history)
    y_score = tfStandardModel.predict_proba(X_val)[:, 1]  # get predicted probabilities for the positive class
    y_true = np.argmax(y_val, axis=1)  # get true labels
    tfStandardModel.plot_roc_curve(y_true, y_score)

    accuracy= tfStandardModel.score(testData=(X_val, y_val))
    print(f"Accuracy: {accuracy * 100:.2f}%")

    # Save the standard model if we want to do further train later
    tfStandardModel.save(modelDir, modelName)
    
    # Convert this model to Tensorflow Lite version
    tfStandardModel.tfline_convert(modelDir, modelName)
    
    #####################################################################################
    # Re-run everything with Tensorflow Lite
    # Extract testing dataset to 40 samples chunk of data
    # Conver from float64 to float32 since tfline only accepts input of float32
    X_test = X_val.astype(np.float32)
    # Convert back to 1 column of value since the tensorflow lite only take the input out (N,)
    # instead of (N, 2) like the standard tensorflow
    y_test = np.argmax(y_val, axis=1)

    modelTfliteSourcePath = f'{modelDir}/{modelName}.tflite'

    # Evaluate both tensorflow lite and rulebased model
    tfLiteModel = fallDetectionTflite(modelTfliteSourcePath)

    if _USE_TFLITE_INTERPRETER.value:
        print('Run standard Tensorflow Lite for desktop and mobile device')
        # y_predictions = tfLiteModel.get_tflite_prediction(X_test)
        accuracy = tfLiteModel.evaluate_accuracy(X_test, y_test)
        print('Accuracy of Tflite mode: ', accuracy)
    else:
        print('Run optimized Tensorflow Lite for microcontroller')
        y_predictions = tfLiteModel.get_tflm_prediction(X_test)

if __name__ == '__main__':
  app.run(main)
