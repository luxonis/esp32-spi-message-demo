import cv2
import depthai as dai
from time import sleep

'''
Speed benchmark. Transfers JPEG encoded 1080P frames to ESP32
'''

print("Creating SPI pipeline: ")
print("RGB.video -> VideoEncoder -> SPIOut")

# Define pipeline
pipeline = dai.Pipeline()
cam_color = pipeline.createColorCamera()
spiout_preview = pipeline.createSPIOut()
videnc = pipeline.createVideoEncoder()

# set up color camera and link to NN node
cam_color.setResolution(dai.ColorCameraProperties.SensorResolution.THE_1080_P);

# VideoEncoder
videnc.setDefaultProfilePreset(1920, 1080, 30, dai.VideoEncoderProperties.Profile.MJPEG);

# Link nodes
cam_color.video.link(videnc.input);
spiout_preview.setStreamName("spipreview");
spiout_preview.setBusId(0);
videnc.bitstream.link(spiout_preview.input);

# Start device
with dai.Device(pipeline) as device:
    while not device.isClosed():
        sleep(1)
