import matplotlib.pyplot as plt
import numpy as np
from scipy.spatial.transform import Rotation as R
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Poly3DCollection, Line3DCollection
import imageio
from matplotlib.offsetbox import OffsetImage, AnnotationBbox
from mpl_toolkits.mplot3d import proj3d

class VisualizeData(object):    
    def __init__(self, plottype):
        self.PACKET_HEADER_SIZE = 2
        self.labels = ['X','Y','Z']
        self.legacyData = [[0] * 200 for _ in range(10)]

        self.cmap = plt.cm.Set1
        # Initialize the plot
        if plottype == 'time':
            self.fig, self.axs = plt.subplots(3, 1, figsize=(17, 7), sharex=True)
        elif plottype == 'frequency':
            self.fig, self.axs = plt.subplots(2, 3, figsize=(18, 10), sharex=True)


    def plotTimeData(self, data):
        time_x = [ii[1]/1000 for ii in data]
        self.legacyData[0][:175] = self.legacyData[0][25:] # shift the data by 25 positions
        self.legacyData[0][175:] = time_x # set the last 25 values to 0

        # plot accelerometer data
        self.axs[0].clear()            
        for zz in range(2, 5):
            data_vec = [ii[zz] for ii in data]
            self.legacyData[zz-1][:175] = self.legacyData[zz-1][25:] # shift the data by 25 positions
            self.legacyData[zz-1][175:] = data_vec # set the last 25 values to 0
            self.axs[0].plot(self.legacyData[0], self.legacyData[zz-1], label=self.labels[zz-2], color=self.cmap(zz))
        self.axs[0].legend(fontsize=10, loc='upper right')
        self.axs[0].set_ylabel('Acceleration [mg]',fontsize=12)

        # plot gyroscope data
        self.axs[1].clear()
        for zz in range(5, 8):
            data_vec = [ii[zz] for ii in data]
            self.legacyData[zz-1][:175] = self.legacyData[zz-1][25:] # shift the data by 25 positions
            self.legacyData[zz-1][175:] = data_vec
            self.axs[1].plot(self.legacyData[0], self.legacyData[zz-1], label=self.labels[zz-5], color=self.cmap(zz-5))
        self.axs[1].legend(fontsize=10, loc='upper right')
        self.axs[1].set_ylabel('Angular Vel. [dps]',fontsize=12)

        # plot magnetometer data
        self.axs[2].clear()
        for zz in range(8, 11):
            data_vec = [ii[zz] for ii in data]
            self.legacyData[zz-1][:175] = self.legacyData[zz-1][25:] # shift the data by 25 positions
            self.legacyData[zz-1][175:] = data_vec
            self.axs[2].plot(self.legacyData[0], self.legacyData[zz-1], label=self.labels[zz-8], color=self.cmap(zz-8))

        self.axs[2].legend(fontsize=10, loc='upper right')
        self.axs[2].set_ylabel('Magn. Field [Î¼T]',fontsize=12)
        self.axs[2].set_xlabel('Time [s]',fontsize=14)

        plt.xticks(rotation=45, ha='right')
        self.fig.align_ylabels(self.axs)

        plt.draw()
        plt.pause(0.1) # pause for 0.1 seconds


    def plotSpectrum(self, data):
        for zz in range(2, 5):
            self.axs[0, zz-2].clear()            
            data_vec = [ii[zz] for ii in data]
            fft_data = np.fft.rfft(data_vec)
            fft_data = np.abs(fft_data) / len(fft_data)
            self.axs[0, zz-2].plot(fft_data, color=self.cmap(zz))
            self.axs[0, zz-2].set_ylabel(f'Acceleration [mg] {self.labels[zz-2]}',fontsize=12)
            self.axs[0, zz-2].set_xlabel('Frequency',fontsize=10)

        for zz in range(5, 8):
            self.axs[1, zz-5].clear()            
            data_vec = [ii[zz] for ii in data]
            fft_data = np.fft.rfft(data_vec)
            fft_data = np.abs(fft_data) / len(fft_data)
            self.axs[1, zz-5].plot(fft_data, color=self.cmap(zz))
            self.axs[1, zz-5].set_ylabel(f'Angular [dps] {self.labels[zz-5]}',fontsize=12)
            self.axs[1, zz-5].set_xlabel('Frequency',fontsize=10)

        plt.xticks(rotation=45, ha='right')
        self.fig.align_ylabels(self.axs)

        plt.draw()
        plt.pause(0.1) # pause for 0.1 seconds


    def clearPlot(self):
        plt.clf()
