/*
The MIT License (MIT)

Copyright (c) 2016 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "SimpleSerial.h"

// debug macro.
#if 0
#define SscLog(msg) Serial.println(msg)
#define SscLogf(msg) Serial.print(msg)
#else
#define SscLog(msg) (void)0
#define SscLogf(msg) (void)0
#endif

void SimpleSerial::begin(long baudrate) {
    pos = 0;
    memset(buf, 0, 16);
    
    Serial.begin(baudrate);
}

bool SimpleSerial::available() {
    if (pos >= 16) {
        return true;
    }
    
    // try to read all data.
    while (pos < 16 && Serial.available()) {
        byte v = Serial.read();
    
        // wait for sync word 0x47.
        if (pos == 0 && v != 0x47) {
            SscLog("Drop for invalid sync word.");
            continue;
        }
    
        buf[pos++] = v;
        
        // wait for data to incomming.
        if (Serial.available() <= 0) {
            delay(10);
        }
    }
    
    // data not fullfill the buffer, please retry.
    if (pos < 16) {
        return false;
    }
    
    return true;
}

int SimpleSerial::read() {
    if (!available()) {
        return -1;
    }
    
    pos = 0;
    return 0;
}

byte SimpleSerial::command()
{
    return buf[1];
}

byte SimpleSerial::arg0()
{
    return buf[2];
}

byte SimpleSerial::arg1()
{
    return buf[3];
}

byte SimpleSerial::arg2()
{
    return buf[4];
}

byte SimpleSerial::arg3()
{
    return buf[5];
}

int SimpleSerial::write0(byte command) {
    return write(command, NULL);
}

int SimpleSerial::write1(byte command, byte arg0) {
    byte b[12] = {0};
    memcpy(b, 0, 12);
    
    b[0] = arg0;
    return write(command, b);
}

int SimpleSerial::write2(byte command, byte arg0, byte arg1) {
    byte b[12] = {0};
    memcpy(b, 0, 12);
    
    b[0] = arg0;
    b[1] = arg1;
    return write(command, b);
}

int SimpleSerial::write3(byte command, byte arg0, byte arg1, byte arg2) {
    byte b[12] = {0};
    memcpy(b, 0, 12);
    
    b[0] = arg0;
    b[1] = arg1;
    b[2] = arg2;
    return write(command, b);
}

int SimpleSerial::write4(byte command, byte arg0, byte arg1, byte arg2, byte arg3) {
    byte b[12] = {0};
    memcpy(b, 0, 12);
    
    b[0] = arg0;
    b[1] = arg1;
    b[2] = arg2;
    b[3] = arg3;
    return write(command, b);
}

int SimpleSerial::write(byte command, byte data[12]) {
    byte b[16] = {0};
    memcpy(b, 0, 16);
    
    b[0] = 0x47;
    b[1] = command;
    if (data) {
        memcpy(b+2, data, 12);
    }
    b[14] = '\n';
    b[15] = '\n';
    
    for (int i = 0; i < 16; i++) {
        if (Serial.write(b[i]) != 1) {
            return -1;
        }
    }
    
    return 0;
}
