import sys
from utils.Serial import Serial
from utils.visualize_data import VisualizeData
import csv

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

# Only use for this function, will optimize later
dataDir = './data_collection'
def logData(path, header, data):
    with open(path, 'w', encoding='UTF8', newline='') as f:
        writer = csv.writer(f)
        # write the header
        writer.writerow(header)
        # write the data
        writer.writerows(data)

# Define serial port for SPP Bluetooth, usually COM6 on window
COM_PORT = '/dev/rfcomm0'

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python imu_data_analysis.py <logging|timeplot|frequencyplot>")
        sys.exit(1)
    serial = Serial(COM_PORT, 115200)
    # Clear all dummy data within the serial buffer
    serial.emptySerial()

    input_arg = sys.argv[1]
    if input_arg == 'logging':
        fileCnt = 0
        lockedSample = 4000
    elif input_arg == 'timeplot':
        plot = VisualizeData('time')
        # Sliding window will be 25 samples
        lockedSample = 25
    elif input_arg == 'frequencyplot':
        # Want to plot spectrum to check noise here
        plot = VisualizeData('frequency')
        previousTime = 0
        lockedFirstTime = True
    elif input_arg == '3D':
        plot = VisualizeData('3D')
        lockedSample = 100
    else:
        print(f"Unknown argument: {input_arg}")

    recoredData = []

    while serial.available():  
        data, cnt = serial.getData(10)
        if data != b'':
            if input_arg == 'logging':
                # We want to log raw data into csv file
                parseIMUData(data)
                csvData = [imuData[name] for name in imuData]
                recoredData.append(csvData)
                if len(recoredData) >= lockedSample:
                    header = [i for i in imuData]
                    fpath = f'{dataDir}/imu_{fileCnt}.csv'
                    logData(fpath, header, recoredData)
                    fileCnt += 1
                    recoredData = []
            else:
                # Read data from the IMU sensor and append to recoredData
                parseIMUData(data, getRAW=False)
                singleSample = [imuData[name] for name in imuData]
                recoredData.append(singleSample)
                if input_arg == 'timeplot':
                    if len(recoredData) % lockedSample == 0:
                        plot.plotTimeData(recoredData)
                        recoredData = []
                elif input_arg == 'frequencyplot':                    # Plot spectrum each 1 second
                    if lockedFirstTime:
                        lockedFirstTime = False
                        previousTime = imuData['time_stamp']

                    if imuData['time_stamp'] - previousTime > 1000:
                        plot.plotSpectrum(recoredData)
                        recoredData = []
                        previousTime = imuData['time_stamp']

        else:
            if len(recoredData) and (input_arg == 'logging'):
                header = [i for i in imuData]
                fpath = f'{dataDir}/imu_{fileCnt}.csv'
                logData(fpath, header, recoredData)
            exit(1)
