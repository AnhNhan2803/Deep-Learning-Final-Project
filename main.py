import sys
import csv
from absl import app
import numpy as np
import select
from utils.fall_detection_algorithms import fallDetectionTflite
from utils.Serial import Serial
from utils.dsp import filterSignal

imuData = {
    'sequence_number': 0,
    'time_stamp':0,

    'data_accelX':0,
    'data_accelY':0,
    'data_accelZ':0,

    'data_gyroX':0,
    'data_gyroY':0,
    'data_gyroZ':0,

    'data_magX':0,
    'data_magY':0,
    'data_magZ':0,
}

def parseIMUData(data, verbose=False, getRAW=True):
    accelResolution = 16384 # 2G
    gyroResolution = 131     # 250dps
    magResolution = 1/0.15
    resolution = [accelResolution, gyroResolution, magResolution]
    idx = 0
    imuData['sequence_number'] = int.from_bytes(data[idx:idx+1], "little")
    idx = 1
    imuData['time_stamp'] = int.from_bytes(data[idx:idx+4], "little")
    if verbose:
        print(f'sequence_number - ', imuData['sequence_number'])
        print(f'time_stamp - ', imuData['time_stamp'])
    idx = 5
    for name in imuData:
        if 'data' in name:
            if getRAW:
                imuData[name] = int.from_bytes(data[idx:idx+2], byteorder="little", signed=True)
            else:
                imuData[name] = int.from_bytes(data[idx:idx+2], byteorder="little", signed=True)/resolution[int((idx-5)/6)]
            idx += 2
            if verbose:
                print(f'{name} - {imuData[name]}')


# A function to detect a key press, mainly using in detecting the start/stop streaming condition of IMU streaming
def key_pressed():
    dr, dw, de = select.select([sys.stdin], [], [], 0)
    return dr != []



def main(_):
    # Define serial port for SPP Bluetooth, usually COM6 on window
    serialPort = '/dev/rfcomm0'
    windowSize = 40
    stride = 1 # Stride 1 samples each time
    fallThresHold = 4

    serial = Serial(serialPort, 115200)
    # Clear all dummy data within the serial buffer
    serial.emptySerial()

    # Need to apply the absolute path since the bazel-bin can not recognize the relative path when running the script, even setting path in .bashrc
    modelTfliteSourcePath = '/home/nhan/gitwork/tflite-micro/tensorflow/lite/micro/examples/imu_fall_detection/model/fall_detection_model.tflite'
    # Since we run this on PC, only support standard tensoreflow lite (tflite), will support tensorflow lite microcontroller
    # (tflm) when porting the model to light weight microcontroller    
    imuAlgo = fallDetectionTflite(modelTfliteSourcePath)

    imuDataWindow = []
    fs = 31.25 # config the sampling rate of IMU 31.25Hz
    filterOrder = 2
    fcutoffHigh = 15 # 15Hz to remove the high frequency noise
    fcutoffLow = 1 # 1Hz Low cut off frequency to remove the gravity
    consecutiveFall = 0

    while serial.available():  
        data,_ = serial.getData(1)

        if data != b'':
            parseIMUData(data, getRAW=False)

            # Append only timestamp + accel + gyro
            imuDataWindow.append([imuData[name] for name in imuData if ('time' in name or 'accel' in name or 'gyro' in name)])
            # Update the downsampling time reference

            if len(imuDataWindow) >= windowSize:                
                # Ignore the timestamp element and get the next six elements in column
                # Also use the expand_dims to expend the number of sample, from (25,3) to (1,25,3)
                # to satisfy the shape of model input
                prefilteredData = np.expand_dims(np.array(imuDataWindow)[:, 1:4] * 9.81, axis=0)

                # Also convert from FLOAT64 to FLOAT32 incase using tensorflow lite, since tflite only accept FLOAT32 as input
                prefilteredData = filterSignal(prefilteredData, 'lowpass', fcutoffHigh, fs, filterOrder)
                prefilteredData = filterSignal(prefilteredData, 'highpass', fcutoffLow, fs, filterOrder)
                processedData = prefilteredData.astype(np.float32)

                fall = imuAlgo.detectFall(processedData)

                if fall:
                    consecutiveFall += 1
                    # Consider as fall with 4 consecutive window of fall detection
                    if consecutiveFall == fallThresHold:
                        consecutiveFall = 0
                        # Log the start time of the data chuck if detecting fall
                        print(f'=========> Fall detected at {imuDataWindow[0][0]} ms')
                else:
                    # Just reset the the counter of consecutive fall
                    consecutiveFall = 0

                # Stride = 1, remove the first sample
                imuDataWindow = imuDataWindow[stride: windowSize]

        if key_pressed():
            key = sys.stdin.read(1)
            print(f"Key pressed: {key}")
            if key == 's':
                serial.sendData(key.encode('utf-8'))
            elif key == 'e':
                serial.sendData(key.encode('utf-8'))
                exit(1)


if __name__ == '__main__':
  app.run(main)