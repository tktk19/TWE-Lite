#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
import sys

argv = sys.argv
argc = len(argv)

def read_serial(s):
        while True:
                line = s.readline()
                print line.strip()

# USBデバイスの指定
if argc != 2:
        s = serial.Serial("/dev/ttyUSB0", 115200, timeout=10)
else :
        s = serial.Serial(argv[1], 115200, timeout=10)

while True:
        try:
                read_serial(s)
        except KeyboardInterrupt:
                break
        except:
                continue

s.close()
