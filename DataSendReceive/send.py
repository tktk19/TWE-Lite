#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
import sys
import select
import time

argv = sys.argv
argc = len(argv)

def digital_write(child_id, pin, value):
	header = get_send_header(child_id)

	if pin == 1:
	        if value == HIGH:
	                cmd = header + '0101'+'FFFFFFFFFFFFFFFF'
	        if value == LOW:
	                cmd = header + '0001'+'FFFFFFFFFFFFFFFF'
	if pin == 2:
	        if value == HIGH:
	                cmd = header + '0202'+'FFFFFFFFFFFFFFFF'
	        if value == LOW:
	                cmd = header + '0002'+'FFFFFFFFFFFFFFFF'
	if pin == 3:
	        if value == HIGH:
	                cmd = header + '0404'+'FFFFFFFFFFFFFFFF'
	        if value == LOW:
	                cmd = header + '0004'+'FFFFFFFFFFFFFFFF'

	# コマンド送信 パリティを省略
    # SDKマニュアルを参照
	s.write(cmd + 'XX\r\n')

def get_send_header(child_id):
	if child_id == '':
		# 全ての子機を対象にする
		header = ':78'
		# コマンド番号 + 書式バージョンは固定値
		header = header + '8001'
	else :
		if child_id >= 1 and child_id <= 100:
			# 特定の子機を対象にする
			header = ':' + '%02x' % int(child_id)
			# コマンド番号 + 書式バージョンは固定値
			header = header + '8001'
		else :
			sys.exit("child_idの指定は1-100までの数値で指定して下さい")

	return header

# main
HIGH = 1
LOW = 0

# USBデバイスの指定
if argc != 2:
	s = serial.Serial("/dev/ttyUSB0", 115200, timeout=10)
else :
	s = serial.Serial(argv[1], 115200, timeout=10)

while True:
	# 全ての子機を対象
	digital_write('', 1, HIGH)
	digital_write('', 2, HIGH)
	digital_write('', 3, HIGH)

	# 特定のidの機器を対象
	digital_write(2, 1, HIGH)
	digital_write(2, 2, HIGH)
	digital_write(2, 3, HIGH)

	time.sleep(0.5)

	digital_write('', 1, LOW)
	digital_write('', 2, LOW)
	digital_write('', 3, LOW)

	time.sleep(0.5)

s.close()
