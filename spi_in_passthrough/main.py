import cv2
import numpy as np
import depthai as dai
import time

'''
This a hello world for the SPIIn node. It basically just returns whatever is sent in right back out.
'''

# Any data that's passed in will be passed back out the SPI interface.
print("Creating SPI pipeline:")
print("SPIIn -> SPIOut")
pipeline = dai.Pipeline()

spiin_nn = pipeline.create(dai.node.SPIIn)
spiin_nn.setStreamName("spiin")
spiin_nn.setBusId(0)

spiout_meta = pipeline.create(dai.node.SPIOut)
spiout_meta.setStreamName("spimeta")
spiout_meta.setBusId(0)

spiin_nn.out.link(spiout_meta.input)

with dai.Device(pipeline) as device:
    while not device.isClosed():
        time.sleep(1)

