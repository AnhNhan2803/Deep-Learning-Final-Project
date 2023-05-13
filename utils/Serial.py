import serial
import time

class Serial(object):    
    def __init__(self, port, baud):
        self.PACKET_HEADER_SIZE = 2
        self.PACKET_FOOTER_SIZE = 2
        self.PACKET_BODY_SIZE = 23
        self.PACKET_TOTAL_SIZE = 27
        self.PACKET_SENSOR_SIZE = 6

        self.serial = serial.Serial(port, baud, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE)

    def sendData(self, buffs):
        self.serial.write(buffs)

    def emptySerial(self):
        self.serial.flushInput()

    def available(self):
        return self.serial.is_open

    def getData(self, timeout):
        receive_data = b''  # serialData is bytes data
        is_enter_header = 0
        received_idx = 0
        start_time = time.time()     

        while True:
            duration = time.time() - start_time
            # Check timeout condition for receiving a packet from MCU
            if duration > timeout:
                # Exceed timeout
                # print('TIME OUT')
                receive_data = b''
                return receive_data, -1
            if self.serial.inWaiting():
                # Accumulate the received data from serial port
                received_idx += 1
                receive_data += self.serial.read()
                # Start collecting data if pass the headers checking
                if is_enter_header == 1:
                    if received_idx == self.PACKET_TOTAL_SIZE:
                        length = self.PACKET_BODY_SIZE
                        # Validate the footer
                        if receive_data[(received_idx - self.PACKET_FOOTER_SIZE):received_idx] != b'\xAD\xDA': 
                            print(f'Invalid frame: {receive_data}')
                            receive_data = b''
                            length = -1
                        return receive_data[self.PACKET_HEADER_SIZE:(self.PACKET_HEADER_SIZE+self.PACKET_BODY_SIZE)], int(length)
                # Check if entered data frame
                if is_enter_header == 0 and received_idx == self.PACKET_HEADER_SIZE:
                    if receive_data == b'\xAB\xBA':
                        is_enter_header = 1
                    else:
                        # Minus the received index to 1 for the next time checking header
                        received_idx -= 1
                        receive_data = receive_data[1:self.PACKET_HEADER_SIZE]
