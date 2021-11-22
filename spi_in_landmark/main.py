import cv2
import numpy as np
import depthai as dai
import time
import blobconverter

'''
This demo will run face landmarks inference on frames recieved from the SPI and return metadata back to the MCU.
SPIIn -> NeuralNetwork -> SPIOut
'''

pipeline = dai.Pipeline()

spiin_nn = pipeline.create(dai.node.SPIIn)
spiin_nn.setStreamName("spiin")
spiin_nn.setBusId(0)

nn1 = pipeline.create(dai.node.NeuralNetwork)
nn1.setBlobPath(str(blobconverter.from_zoo('landmarks-regression-retail-0009', shaves=6, version='2021.4')))
spiin_nn.out.link(nn1.input)

spiout_meta = pipeline.create(dai.node.SPIOut)
spiout_meta.setStreamName("spimeta")
spiout_meta.setBusId(0)
nn1.out.link(spiout_meta.input)

with dai.Device(pipeline) as device:
    while not device.isClosed():
        time.sleep(1)
