import os
import time
import numpy as np
import matplotlib.pyplot as plt
import tensorflow as tf
from pathlib import Path
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Conv2D, MaxPooling2D, Dense, Flatten, Dropout
from tflite_micro.tensorflow.lite.micro.python.interpreter.src import tflm_runtime
from tensorflow.keras.models import load_model
from tensorflow.keras import losses
from sklearn.metrics import roc_curve, auc

# Original Tensorflow version of the fall detection model
class fallDetectionStandard(object):
    def __init__(self, modelPath=None, windowSize=10, kernelSize=4, layersActivation='relu', 
                 finalActivation='softmax', dropOutRate=0.2, poolSize=2, verbose=False):
        self.windowSize = windowSize
        self.verbose = verbose

        if modelPath is None:
            # Define a new model
            self.model = Sequential()
            # These hyperparameters seems to be the optimal values due to the Random Search in fallDetectionHyperModel
            self.model.add(Conv2D(32, (kernelSize, 1), input_shape=(self.windowSize, 3, 1), activation=layersActivation))
            self.model.add(Dropout(dropOutRate))
            self.model.add(MaxPooling2D(pool_size = (poolSize, 1)))
            self.model.add(Conv2D(32, (kernelSize, 1), activation=layersActivation))
            self.model.add(MaxPooling2D(pool_size = (poolSize, 1)))
            # Flaten the output from the second max-pooling
            self.model.add(Flatten())
            self.model.add(Dense(32, activation=layersActivation))
            self.model.add(Dense(8, activation=layersActivation))
            self.model.add(Dropout(dropOutRate))
            self.model.add(Dense(2, activation=finalActivation))
        else:
            self.model = load_model(modelPath)
            
        if self.verbose:
            self.model.summary()

    def train(self, trainData, validationData):
        # Check available GPU, if yes then limit to the fist core of GPU
        gpus = tf.config.experimental.list_physical_devices('GPU')
        if gpus:
            # Restrict TensorFlow to only use the first GPU
            try:
                tf.config.experimental.set_visible_devices(gpus[0], 'GPU')
                logical_gpus = tf.config.experimental.list_logical_devices('GPU')
                print(len(gpus), "Physical GPUs,", len(logical_gpus), "Logical GPU")
            except RuntimeError as e:
                # Visible devices must be set at program startup
                print(e)
        
        # Apply Adam optimizer with its default learning rate, which is 0.001. The Adam optimizer adjusts 
        # the learning rate adaptively, then te,porarily no need to take care of the learning rate
        self.model.compile(
            # loss='categorical_crossentropy',
            loss=losses.BinaryCrossentropy(),
            optimizer='adam',
            metrics=['accuracy']
        )

        class_weight = {0: 1., 1: 10.}

        # Convert data from tuple, then fit data to the model
        X_train, y_train = trainData
        X_val, y_val = validationData
        history = self.model.fit(x=X_train, y=y_train, batch_size=100, epochs=20, validation_data=(X_val, y_val), class_weight=class_weight)
        # history = self.model.fit(x=X_train, y=y_train, batch_size=100, epochs=10, validation_data=(X_val, y_val))
        
        return history

    def predict_proba(self, data):
        return self.model.predict(data)
    
    def predict(self, data):
        start_time = time.time()
        ret = np.where(self.predict_proba(data) >= 0.5, 1, 0)
        end_time = time.time()
        inference_time_ms = (end_time - start_time)
        if not self.verbose:
            print("Inference time: {:.2f} ms".format(inference_time_ms))
        return ret
    
    def score(self, testData):
        X_test, y_test = testData
        loss_metrics = self.model.evaluate(X_test, y_test, verbose=0)
        accuracy = loss_metrics[1]
        if self.verbose:
            print(f'Accuracy: {accuracy}\n')
        return accuracy

    
    def save(self, modelDir, modelName):
        self.model.save(f'{modelDir}/{modelName}.h5')

    def tfline_convert(self, tflitedir, tflitename):
        converter = tf.lite.TFLiteConverter.from_keras_model(self.model)
        model_tflite = converter.convert()
        printStatus = Path(f'{tflitedir}/{tflitename}.tflite').write_bytes(model_tflite)
        if self.verbose:
            print(printStatus)
        # Also convert to the tflite binary to c/c++ header file for future use
        os.system(f'xxd -i {tflitedir}/{tflitename}.tflite > {tflitedir}/{tflitename}.h')
        # Since BAZEL will run in the cache directory, then running the above command will
        # cause a very long name of variable in the header file, we can temporarily modify
        # the header file after running the above command

    def plot_metrics(self, history):
        metrics = ['loss', 'accuracy']
        for n, metric in enumerate(metrics):
            name = metric.replace("_"," ").capitalize()
            plt.plot(history.epoch, history.history[metric], color='blue', label='Train')
            plt.plot(history.epoch, history.history['val_'+metric], color='red', linestyle="--", label='Val')
            plt.xlabel('Epoch')
            plt.ylabel(name)
            if metric == 'loss':
                plt.ylim([0, plt.ylim()[1]])
            elif metric == 'accuracy':
                plt.ylim([0.5,1.1])

            plt.legend() 

            # Save the figure, apply absolute path since bazel run in cache and can not detect relative path
            plt.savefig(f'/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/figures/optimal_model_validation/{metric}_plot.png')
            plt.close()  # close the figure

    def plot_roc_curve(self, y_true, y_score):
        # Calculate ROC curve from y_true and y_score
        fpr, tpr, thresholds = roc_curve(y_true, y_score)
        roc_auc = auc(fpr, tpr)

        # Create a figure
        plt.figure()

        # Plot the ROC curve
        plt.plot(fpr, tpr, color='darkorange', lw=2, label='ROC curve (area = %0.2f)' % roc_auc)
        plt.plot([0, 1], [0, 1], color='navy', lw=2, linestyle='--')
        plt.xlim([0.0, 1.0])
        plt.ylim([0.0, 1.05])
        plt.xlabel('False Positive Rate')
        plt.ylabel('True Positive Rate')
        plt.title('Receiver Operating Characteristic')
        plt.legend(loc="lower right")

        # Save the figure, apply absolute path since bazel run in cache and can not detect relative path
        plt.savefig('/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/figures/optimal_model_validation/roc_curve.png')
        plt.close()  # close the figure


# Hyperparameters tuning for the CNN model
from kerastuner import HyperModel

class fallDetectionHyperModel(HyperModel):
    def __init__(self, input_shape, num_classes):
        self.input_shape = input_shape
        self.num_classes = num_classes

    def build(self, hp):
        model = Sequential()
        model.add(Conv2D(
            filters=hp.Int('conv_1_filter', min_value=8, max_value=32, step=8),
            kernel_size=(3, 1),
            activation='relu',
            input_shape=self.input_shape
        ))
        model.add(MaxPooling2D(pool_size=(hp.Choice('pool_1_size', values=[2,3,4]), 1)))
        model.add(Dropout(rate=hp.Float('dropout_1', min_value=0.0, max_value=0.5, step=0.1)))
        model.add(Conv2D(
            filters=hp.Int('conv_2_filter', min_value=8, max_value=32, step=8),
            kernel_size=(3, 1),
            activation='relu'
        ))
        model.add(MaxPooling2D(pool_size=(2, 1)))
        model.add(Dropout(rate=hp.Float('dropout_2', min_value=0.0, max_value=0.5, step=0.1)))
        model.add(Flatten())
        model.add(Dense(
            units=hp.Int('dense_1_units', min_value=16, max_value=32, step=16),
            activation='relu'
        ))
        model.add(Dense(
            units=hp.Int('dense_2_units', min_value=8, max_value=16, step=8),
            activation='relu'
        ))
        model.add(Dense(self.num_classes, activation='softmax'))

        model.compile(optimizer='adam',     
                      loss=losses.BinaryCrossentropy(), 
                      metrics=['accuracy'])
        return model


# Tensorflow lite version of the fall detection model, re-utilized the skeleton from
# hello world example
class fallDetectionTflite(object):
    def __init__(self, modelPath, tflm=False, verbose=False):
        self.verbose = verbose
        self.modelPath = modelPath
        
        if tflm:
            self.tflm_interpreter = tflm_runtime.Interpreter.from_file(self.modelPath)
        else: # Use the standard tensorflow lite instead
            # load TFLite interpreter and allocate tensor
            self.tflite_interpreter = tf.lite.Interpreter(
                model_path=self.modelPath,
                experimental_op_resolver_type=tf.lite.experimental.OpResolverType.
                BUILTIN_REF,
            )
            self.tflite_interpreter.allocate_tensors()

    # Re-utilize from hello_world example, tinyML project
    def invoke_tflm_interpreter(self, input_shape, interpreter, x_value, input_index,
                                output_index):
        input_data = np.reshape(x_value, input_shape)
        interpreter.set_input(input_data, input_index)
        interpreter.invoke()
        y_quantized = np.reshape(interpreter.get_output(output_index), -1)[0]
        return y_quantized

    # Re-utilize from hello_world example, tinyML project but change
    # the way of getting y_output since the standard structure of model
    # will have 2 output of fall and not-fall, two-unit dense layer with 
    # softmax activation, which means that the output of the model should 
    # be a probability distribution over two classes
    def invoke_tflite_interpreter(self, input_shape, interpreter, x_value, input_index,
                                    output_index):
        input_data = np.reshape(x_value, input_shape)
        interpreter.set_tensor(input_index, input_data)
        interpreter.invoke()
        tflite_output = interpreter.get_tensor(output_index)
        y_output = np.argmax(tflite_output, axis=-1)[0]  # This line has been corrected
        return y_output

    # Re-utilize from hello_world example, tinyML project
    def get_tflm_prediction(self, x_values):
        # Create the tflm interpreter
        input_shape = np.array(self.tflm_interpreter.get_input_details(0).get('shape'))

        y_predictions = np.empty(x_values.size, dtype=np.float32)

        for i, x_value in enumerate(x_values):
            y_predictions[i] = self.invoke_tflm_interpreter(input_shape,
                                                        self.tflm_interpreter,
                                                        x_value,
                                                        input_index=0,
                                                        output_index=0)
        return y_predictions

    # Re-utilize from hello_world example, tinyML project
    def get_tflite_prediction(self, x_values):
        input_details = self.tflite_interpreter.get_input_details()[0]
        output_details = self.tflite_interpreter.get_output_details()[0]
        
        input_shape = np.array(input_details.get('shape'))

        y_predictions = np.empty(len(x_values), dtype=np.float32)
        
        for i, x_value in enumerate(x_values):
            start_time = time.time()
            y_predictions[i] = self.invoke_tflite_interpreter(
                input_shape,
                self.tflite_interpreter,
                x_value,
                input_details['index'],
                output_details['index'],
            )
            end_time = time.time()
            inference_time_ms = (end_time - start_time)
            if self.verbose:
                print("Inference time: {:.2f} ms".format(inference_time_ms))
        return y_predictions
    
    # Evaluate the accurary compared to available labels
    def evaluate_accuracy(self, x_values, y_true):
        if hasattr(self, 'tflm_interpreter'):
            y_pred = self.get_tflm_prediction(x_values)
        else:
            y_pred = self.get_tflite_prediction(x_values)
        # Round the predicted values to the nearest integer (0 or 1)
        y_pred_rounded = np.round(y_pred)
        # Calculate the accuracy by comparing the true labels and predicted labels
        accuracy = np.mean(y_pred_rounded == y_true)
        return accuracy
    
    # Get the prediction of fall or not-fall
    def detectFall(self, data):        
        return np.round(self.get_tflite_prediction(data))
