import cv2
import numpy as np
import depthai as dai
import time
import blobconverter

'''
This a hello world for the SPIIn node. It basically just returns whatever is sent in right back out.
'''

# Any data that's passed in will be passed back out the SPI interface.
print("Creating SPI pipeline: ")
print("COLOR CAM -> ENCODER -> SPI OUT")
pipeline = dai.Pipeline()

nn1 = pipeline.create(dai.node.NeuralNetwork)
spiout_meta = pipeline.create(dai.node.SPIOut)
spiin_nn = pipeline.create(dai.node.SPIIn)

spiin_nn.setStreamName("spiin")
spiin_nn.setBusId(0)

nn1.setBlobPath(str(blobconverter.from_zoo('landmarks-regression-retail-0009', shaves=6, version='2021.4')))
spiin_nn.out.link(nn1.input)


spiout_meta.setStreamName("spimeta")
spiout_meta.setBusId(0)

#spiin_nn.out.link(spiout_meta.input)
nn1.out.link(spiout_meta.input)

with dai.Device(pipeline) as device:
    while not device.isClosed():
        time.sleep(1)
