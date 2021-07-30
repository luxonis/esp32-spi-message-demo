#!/usr/bin/env python3
import time
import depthai as dai

# Start defining a pipeline
pipeline = dai.Pipeline()

# Script node
script = pipeline.create(dai.node.Script)
script.setScript("""
    import time
    import GPIO
    # Pin 34, highCount (clock cycles), lowCount (clock cycles)
    ret = GPIO.setup(34, GPIO.OUT)
    node.warn(f'gpio 34 init ret: {ret}')
    node.warn(f'gpio 34 val: {GPIO.read(34)}')
    GPIO.setPwm(34, 100000000, 100000000)
    GPIO.enablePwm(34, 1)
    time.sleep(1)
    GPIO.enablePwm(34, 0)

    # Interrupts
    cb = lambda gpio : node.warn(f'Rising edge interrupted GPIO{gpio}')
    # Default - input, no pull up/down, shared
    node.warn(f'setup ret: {GPIO.setup(32)}')
    node.warn(f'setInterrupt ret: {GPIO.setInterrupt(32, GPIO.RISING, 100, cb)}')
    while True:
        time.sleep(1)
""")

# Connect to device with pipeline
with dai.Device(pipeline) as device:
    while True:
        time.sleep(1)
