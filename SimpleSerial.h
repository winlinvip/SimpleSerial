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

#ifndef __SIMPLE_SERIAL_H
#define __SIMPLE_SERIAL_H

#include <Arduino.h>

/*
    Simple Serial

    Simple, Stable and Fast Arduino Serial Command Protocol.

    The circuit:
    * VCC: 5V
    * GND: GND
    * RX: TX(PIN 1)
    * TX: RX(PIN 0)

    23 Jan 2016 By winlin <winlin@vip.126.com>

    https://github.com/winlinvip/SimpleSerial#usage

*/
class SimpleSerial {
private:
    // each command is 35bytes.
    //      1byte sync word, 0x47 'G'.
    //      1byte command, for example, SSC_PING 'P'.
    //      12byte data bytes.
    //      1byte Reserved '\n'.
    //      1byte EOF '\n'.
    byte buf[16];
    // current received position.
    byte pos;
public:
    void begin(long baudrate);
    bool available();
public:
    int read();
    byte command();
    byte arg0();
    byte arg1();
    byte arg2();
    byte arg3();
public:
    int write0(byte command);
    int write1(byte command, byte arg0);
    int write2(byte command, byte arg0, byte arg1);
    int write3(byte command, byte arg0, byte arg1, byte arg2);
    int write4(byte command, byte arg0, byte arg1, byte arg2, byte arg3);
public:
    int write(byte command, byte data[12]);
};

// Simple Serial Commands.
enum SimpleSerialCommands {
    // ping peer.
    // GP01234567890123
    SSC_PING = 'P',
    // query the temperature and humidity.
    // GQ01234567890123
    SSC_QUERY_TH = 'Q',
    SSC_RESP_TH = 'R',
    // not support command, arg0 is the command.
    // GNA0123456789012
    SSC_NOT_SUPPORT = 'N',
    // open the heater, arg0 is the target temperature, arg1 is the timeout in seconds.
    // GH0Z012345678901
    // where the 0 is 48*C, Z is 90 seconds.
    SSC_OPEN_HEATER = 'H',
    // the heater is opened.
    // GI01234567890123
    SSC_HEATER_OPENED = 'I',
    // the heater is closed, arg0 is target temperature, arg1 is the timeout in seconds, 
    // arg2 is current temperature, arg3 is whether timeout.
    // GCTTTT0123456789
    SSC_HEATER_CLOSED = 'C',
    // open the fan, arg0 is target humidity, arg1 is the timeout in seconds.
    // GO0Z012345678901
    SSC_OPEN_FAN = 'O',
    // the fan is opened. 
    // GF01234567890123
    SSC_FAN_OPENED = 'F',
    // the fan is stopped, arg0 is target humidity, arg1 is the timeout in seconds, 
    // arg2 is current humidity, arg3 is whether timeout.
    // GSTTTT0123456789
    SSC_FAN_CLOSED = 'S',
};

#endif
