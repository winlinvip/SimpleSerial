#!/usr/bin/env python

import time

# read https://github.com/pyserial/pyserial
import serial

def open(device="/dev/ttyUSB0", baudrate=115200):
    f = SimpleSerial()
    f.open(device, baudrate)
    return f

SSC_PING = 'P'
SSC_QUERY_TH = 'Q'
SSC_RESP_TH = 'R'
SSC_NOT_SUPPORT = 'N'
def str(command):
    if command == SSC_PING:
        return "Ping"
    elif command == SSC_QUERY_TH:
        return "QueryTH"
    elif command == SSC_RESP_TH:
        return "ResponseTH"
    elif command == SSC_NOT_SUPPORT:
        return "NotSupported"
    else:
        return "Unknown(%s)"%(command)

class SimpleSerial:
    def __init__(self):
        # the transport.
        self.f = None
        # each command is 35bytes.
        #      1byte sync word, 0x47 'G'.
        #      1byte command, for example, SSC_PING 'P'.
        #      12byte data bytes.
        #      1byte Reserved '\n'.
        #      1byte EOF '\n'.
        self.buf = []
        for i in range(16):
            self.buf.append(0)
        # current received position.
        self.pos = 0
    def open(self, device, baudrate):
        self.f = serial.Serial(device, baudrate)
    # @return bool
    def available(self):
        if self.pos >= 16:
            return True
    
        # try to read all data.
        while self.pos < 16 and self.f.inWaiting() > 0:
            v = self.f.read(1)
            #print("Got byte %s"%(v))
            
            # wait for sync word 0x47.
            if self.pos == 0 and v != 'G':
                #print("Drop for invalid sync word.")
                continue
                
            self.buf[self.pos] = v
            self.pos += 1
            
            if self.f.inWaiting() <= 0:
                time.sleep(0.01)
                
        # data not fullfill the buffer, please retry.
        if self.pos < 16:
            return False
        return True
    # @return command
    def read0(self):
        if self.available() == False:
            raise Exception("No available data")
        self.pos = 0
        return self.buf[1]
    # command in str, args in int.
    # @return tuple(command, arg0)
    def read1(self):
        return (self.read0(), ord(self.buf[2]))
    # @return tuple(command, arg0, arg1)
    def read2(self):
        return (self.read0(), ord(self.buf[2]), ord(self.buf[3]))
    # @return tuple(command, arg0, arg1, arg2)
    def read3(self):
        return (self.read0(), ord(self.buf[2]), ord(self.buf[3]), ord(self.buf[4]))
    # @return tuple(command, data[12])
    def read(self):
        v = []
        for i in range(self.buf[2:14]):
            v.append(ord(i))
        return (self.read0(), v)
    def write0(self, command):
        self.write(command, None)
    def write1(self, command, arg0):
        self.write(command, [arg0])
    def write2(self, command, arg0, arg1):
        self.write(comand, arg0, arg1)
    def write3(self, command, arg0, arg1, arg2):
        self.write(comand, arg0, arg1, arg2)
    # @param data is byte[12]
    def write(self, command, data):
        b = []
        
        b.append(chr(0x47))
        b.append(command)
        
        if data is not None:
            for i in range(len(data)):
                b.append(data[i])
        for i in range(len(b), 14, 1):
            b.append(chr(0))
            
        b.append('\n')
        b.append('\n')
        
        #print("Write %s"%(bytes(b)))
        for c in b:
            self.f.write(c)
        self.f.flush()
        