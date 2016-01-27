#!/usr/bin/env python

import time

# read https://github.com/pyserial/pyserial
import serial

def open(device="/dev/ttyUSB0", baudrate=115200):
    f = SimpleSerial()
    f.open(device, baudrate)
    return f
    
def ping():
    return Command('P')
def queryTH():
    return Command('Q')
def respTH(t, h):
    return Command('R').set_arg1(t, h)
def openHeater(t, e):
    return Command('H').set_arg1(t, e)
def heaterOpened():
    return Command('I')
def heaterClosed(tt, te, t, e):
    return Command('C').set_arg3(tt, te, t, e)
def openFan(h, e):
    return Command('F').set_arg1(h, e)
def fanOpened():
    return Command('O')
def fanClosed(ht, he, h, e):
    return Command('S').set_arg3(ht, he, h, e)
def notSupported(c):
    return Command('N').set_arg0(c)
            
def is_ping(cmd):
    return cmd.command == 'P'
def is_query_th(cmd):
    return cmd.command == 'Q'
def is_resp_th(cmd):
    return cmd.command == 'R'
def is_open_heater(cmd):
    return cmd.command == 'H'
def is_heater_opened(cmd):
    return cmd.command == 'I'
def is_heater_closed(cmd):
    return cmd.command == 'C'
def is_open_fan(cmd):
    return cmd.command == 'F'
def is_fan_opened(cmd):
    return cmd.command == 'O'
def is_fan_closed(cmd):
    return cmd.command == 'S'
def is_not_supported(cmd):
    return cmd.command == 'N'
def is_unknown(cmd):
    return cmd.command not in ['P', 'Q', 'R', 'H', 'I', 'C', 'F', 'O', 'S', 'N']
        
def parse(command, args):
    return Command(command, args)
        
def str(cmd):
    command = cmd.command
    if command == 'P':
        return 'Ping'
    elif command == 'Q':
        return 'QueryTH'
    elif command == 'R':
        return "ResponseTH"
    elif command == 'H':
        return "OpenHeater"
    elif command == 'I':
        return "HeaterOpened"
    elif command == 'C':
        return "HeaterClosed"
    elif command == 'F':
        return "FanOpened"
    elif command == 'O':
        return "OpenFan"
    elif command == 'S':
        return "FanClosed"
    elif command == 'N':
        return "NotSupported(%s)"%(cmd.args[0])
    else:
        return "Unknown(%s)(%s)"%(cmd.command, cmd.args)

class Command:
    # the buf[1] of buf[16], the command byte.
    # the buf[2:14] of buf[16], the args.
    # command in str, args in int.
    def __init__(self, command, args=None):
        self.command = command
        self.args = args
        
        if args is not None:
            return
        self.args = []
        for i in range(12):
            self.args.append(chr(0))
            
    def str(self):
        return str(self)
            
    # @return command
    def arg0(self):
        return ord(self.args[0])
    def set_arg0(self, arg0):
        self.args[0] = chr(arg0)
        return self
        
    # @return tuple(command, arg0)
    def arg1(self):
        return (ord(self.args[0]), ord(self.args[1]))
    def set_arg1(self, arg0, arg1):
        self.args[0] = chr(arg0)
        self.args[1] = chr(arg1)
        return self
        
    # @return tuple(command, arg0, arg1)
    def arg2(self):
        return (ord(self.args[0]), ord(self.args[1]), ord(self.args[2]))
    def set_arg2(self, arg0, arg1, arg2):
        self.args[0] = chr(arg0)
        self.args[1] = chr(arg1)
        self.args[2] = chr(arg2)
        return self
        
    # @return tuple(command, arg0, arg1, arg2)
    def arg3(self):
        return (ord(self.args[0]), ord(self.args[1]), ord(self.args[2]), ord(self.args[3]))
    def set_arg3(self, arg0, arg1, arg2, arg3):
        self.args[0] = chr(arg0)
        self.args[1] = chr(arg1)
        self.args[2] = chr(arg2)
        self.args[3] = chr(arg3)
        return self

class SimpleSerial:
    def __init__(self):
        # the transport.
        self.f = None
        # each command is 35bytes.
        #      1byte sync word, 0x47 'G'.
        #      1byte command, for example, __SSC_PING 'P'.
        #      12byte data bytes.
        #      1byte Reserved '\n'.
        #      1byte EOF '\n'.
        self.buf = []
        for i in range(16):
            self.buf.append(0)
        # current received position.
        self.pos = 0
        # last recv command.
        self.lrecv = None
        # last send command.
        self.lsend = None
        
    def open(self, device, baudrate):
        self.f = serial.Serial(device, baudrate)
        
    # @return bool
    def available(self, timeout=None):
        expired = None
        if timeout is not None:
            expired = time.time() + timeout
            
        while True:
            # got data.
            if self.__available():
                return True
            # expired not used, no data.
            if expired is None:
                return False
            # expired, no data.
            if expired > time.time():
                return False
            # wait and retry.
            time.sleep(0.1)
            
    # do detect available without timeout.
    def __available(self):
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
        
    # @return tuple(command, data[12])
    def read(self):
        if self.available() == False:
            raise Exception("No available data")
        self.pos = 0
        
        command = self.buf[1]
        v = []
        for i in self.buf[2:14]:
            v.append(i)
        
        self.lrecv = parse(command, v)
        return self.lrecv
        
    # @param command object with data is byte[12]
    def write(self, cmd):
        b = []
        
        b.append(chr(0x47))
        b.append(cmd.command)
        
        for i in cmd.args:
            b.append(i)
        while len(b) < 14:
            b.append(chr(0))
            
        b.append('\n')
        b.append('\n')
        
        #print("Write %s"%(bytes(b)))
        for c in b:
            self.f.write(c)
        self.f.flush()
        