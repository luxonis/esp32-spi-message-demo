import cv2
import numpy as np
import depthai as dai
import time

'''
Basic demo of gen2 pipeline builder functionality where output of jpeg encoded images are sent out SPI rather than the typical XLink out interface.

Make sure you have something to handle the SPI protocol on the other end! See the included ESP32 example.
'''

print("Creating SPI pipeline: ")
print("COLOR CAM -> ENCODER -> SPI OUT")
pipeline = dai.Pipeline()

cam_color         = pipeline.createColorCamera()
spiout_preview    = pipeline.createSPIOut()
videnc            = pipeline.createVideoEncoder()

# set up color camera
cam_color.setPreviewSize(300, 300);
cam_color.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1080_P);
cam_color.setInterleaved(False);
cam_color.setColorOrder(dai.ColorCameraProperties.ColorOrder.BGR);

# Link plugins CAM -> ENCODER -> SPI OUT
spiout_preview.setStreamName("spipreview");
spiout_preview.setBusId(0);
cam_color.preview.link(spiout_preview.input);

with dai.Device(pipeline) as device:
    while not device.isClosed():
        time.sleep(1)

