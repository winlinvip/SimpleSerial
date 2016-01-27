#!/usr/bin/env python

import time

# read https://github.com/pyserial/pyserial
import serial

def open(device="/dev/ttyUSB0", baudrate=115200):
    f = SimpleSerial()
    f.open(device, baudrate)
    return f
    
def ping():
    return PingCommand()
def queryTH():
    return QueryTHCommand()
def respTH(t, h):
    return RespTHCommand().set_arg1(t, h)
def openHeater(t, e):
    return OpenHeaterCommand().set_arg1(t, e)
def heaterClosed(tt, te, t, e):
    return HeaterClosedCommand().set_arg3(tt, te, t, e)
def notSupported(c):
    return NotSupportedCommand().set_arg0(c)
        
def parse(command, args):
    if command == 'P':
        return PingCommand(args)
    elif command == 'Q':
        return QueryTHCommand(args)
    elif command == 'R':
        return RespTHCommand(args)
    elif command == 'H':
        return OpenHeaterCommand(args)
    elif command == 'I':
        return HeaterOpenedCommand(args)
    elif command == 'C':
        return HeaterClosedCommand(args)
    elif command == 'N':
        return NotSupportedCommand(args)
    else:
        return UnknownCommand(command, args)

class Command:
    # the buf[1] of buf[16], the command byte.
    # the buf[2:14] of buf[16], the args.
    # command in str, args in int.
    def __init__(self, command, args):
        self.command = command
        self.args = args
        
        if args is not None:
            return
        self.args = []
        for i in range(12):
            self.args.append(chr(0))
            
    def is_ping(self):
        return False
    def is_query_th(self):
        return False
    def is_resp_th(self):
        return False
    def is_open_heater(self):
        return False
    def is_heater_opened(self):
        return False
    def is_heater_closed(self):
        return False
    def is_not_supported(self):
        return False
    def is_unknown(self):
        return False
            
    def str(self):
        return "Unknown(%s)(%s)"%(self.command, self.args)
            
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

class PingCommand(Command):
    def __init__(self, args=None):
        Command.__init__(self, 'P', args)
    def is_ping(self):
        return True
    def str(self):
        return "Ping"
        
class QueryTHCommand(Command):
    def __init__(self, args=None):
        Command.__init__(self, 'Q', args)
    def is_query_th(self):
        return True
    def str(self):
        return "QueryTH"
        
class RespTHCommand(Command):
    def __init__(self, args=None):
        Command.__init__(self, 'R', args)
    def is_resp_th(self):
        return True
    def str(self):
        return "ResponseTH"
        
class OpenHeaterCommand(Command):
    def __init__(self, args=None):
        Command.__init__(self, 'H', args)
    def is_open_heater(self):
        return True
    def str(self):
        return "OpenHeater"
        
class HeaterOpenedCommand(Command):
    def __init__(self, args=None):
        Command.__init__(self, 'I', args)
    def is_heater_opened(self):
        return True
    def str(self):
        return "HeaterOpened"
        
class HeaterClosedCommand(Command):
    def __init__(self, args=None):
        Command.__init__(self, 'C', args)
    def is_heater_closed(self):
        return True
    def str(self):
        return "HeaterClosed"
        
class NotSupportedCommand(Command):
    def __init__(self, args=None):
        Command.__init__(self, 'N', args)
    def is_not_supported(self):
        return True
    def str(self):
        return "NotSupported(%s)"%(self.args[0])

class UnknownCommand(Command):
    def __init__(self, command, args=None):
        Command.__init__(self, command, args)
    def is_unknown(self):
        return True
    def str(self):
        return "Unknown(%s, %s)"%(self.command, self.args)

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
            v.append(ord(i))
        
        self.lrecv = Command.parse(command, v)
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
        