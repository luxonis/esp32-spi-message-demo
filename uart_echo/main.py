#!/usr/bin/env python3
import sys, time
import depthai as dai

if len(sys.argv) < 2:
    print('Usage: main.py tty/pyserial')
    exit(0)

# Start defining a pipeline
pipeline = dai.Pipeline()

# Script node
script = pipeline.create(dai.node.Script)
script.setProcessor(dai.ProcessorType.LEON_CSS)

pyserial = """
    import serial
    import time

    # time.sleep(50)

    msg = 'hello world from pyserial'
    node.warn(f'Sending message to UART: {msg}')

    # Open UART3 port and set baudrate
    ser = serial.Serial("/dev/ttyS3", baudrate=115200)

    while True:
        # Send hello message and print back received message
        ser.write(msg.encode())
        node.warn(f'Read: {ser.read(16)}')
        time.sleep(1)
"""

tty = """
    import os, tty
    import time

    # time.sleep(50)

    msg = 'hello world from tty'
    node.warn(f'Sending message to UART: {msg}')

    # Open UART3 port, set raw and baudrate
    fd = os.open("/dev/ttyS3", os.O_RDWR | os.O_NOCTTY)
    tty.setraw(fd)
    mode = tty.tcgetattr(fd)
    mode[tty.ISPEED] = mode[tty.OSPEED] = tty.B115200
    tty.tcsetattr(fd, tty.TCSANOW, mode)

    while True:
        # Send hello message and print back received message
        os.write(fd, msg.encode())
        node.warn(f'Read: {os.read(fd, 16)}')
        time.sleep(1)

"""

if sys.argv[1] == 'tty':
    script.setScript(tty)
else:
    script.setScript(pyserial)

xout = pipeline.create(dai.node.XLinkOut)
xout.setStreamName('end')
script.outputs['end'].link(xout.input)

# Shorthand for GPIO
GPIO = dai.BoardConfig.GPIO
# Create BoardConfig and enable UART3 on pins 34 and 35
config = dai.Device.Config()
# UART TX
config.board.gpio[34] = GPIO(GPIO.OUTPUT, GPIO.ALT_MODE_5)
# UART RX
config.board.gpio[35] = GPIO(GPIO.INPUT, GPIO.ALT_MODE_5)
# Enable UART3
config.board.uart[3] = dai.BoardConfig.UART()

# Connect to device with pipeline
with dai.Device(config) as device:
    device.startPipeline(pipeline)
    while not device.isClosed():
        time.sleep(1)