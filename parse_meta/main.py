import cv2
import sys
import numpy as np
import depthai as dai
import time
import blobconverter

'''
This example attaches a NeuralNetwork node directly to the SPI output. The corresponding ESP32 example shows how to decode it.

Make sure you have something to handle the SPI protocol on the other end! See the included ESP32 example.
'''
print("Creating SPI pipeline: ")
print("COLOR CAM -> DetectionNetwork -> SPI OUT")

pipeline = dai.Pipeline()

# set up NN node
nn1 = pipeline.create(dai.node.MobileNetDetectionNetwork)
nn1.setBlobPath(str(blobconverter.from_zoo('mobilenet-ssd', shaves=6, version='2021.4')))

# set up color camera and link to NN node
colorCam = pipeline.create(dai.node.ColorCamera)
colorCam.setPreviewSize(300, 300)
colorCam.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1080_P)
colorCam.setInterleaved(False)
colorCam.setColorOrder(dai.ColorCameraProperties.ColorOrder.BGR)
colorCam.preview.link(nn1.input)

# set up SPI out node and link to nn1
spiOut = pipeline.create(dai.node.SPIOut)
spiOut.setStreamName("spimetaout")
spiOut.setBusId(0)
spiOut.input.setBlocking(False)
spiOut.input.setQueueSize(2)
nn1.out.link(spiOut.input)


with dai.Device(pipeline) as device:
    while not device.isClosed():
        time.sleep(1)
