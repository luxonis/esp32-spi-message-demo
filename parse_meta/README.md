[中文文档](README.zh-CN.md)

## ESP Side Raw Data Parsing Demo

### Overview:
This demo requires an ESP32 board connected via SPI to the DepthAI. The easiest way to accomplish this is to get a hold of an [OAK IOT device](https://docs.luxonis.com/projects/hardware/en/latest/#iot-designs), as they have an integrated ESP32 already connected to the Myriad X VPU via SPI.

This demo showcases device side result parsing for known network types. Raw results from networks such as YOLO quickly become larger than the ESP32 can handle. To handle this for common network types DetectionNetwork nodes can be used. The DetectionNetwork node takes in a neural network blob and outputs object detection results in a simplified format that the ESP32 can read.

### On the DepthAI:
In main.py, a basic pipeline consisting of just 3 nodes is built and sent over to the DepthAI. 
1. The preview from the ColorCamera node is passed into a YoloDetectionNetwork node.
2. The YoloDetectionNetwork node runs YOLO and parses the results.
3. The SPIOut Node recieves parsed results from YoloDetectionNetwork and sends them to the ESP32 via SPI.

### On the ESP32:
The ESP32 is running a custom protocol to communicate over SPI with the DepthAI. This protocol is hidden behind a simple API that lives in components/depthai-spi-api. 

In this example the ESP32 receives parsed simplified results from YOLO running on the DepthAI. The ESP32 simply decodes and prints the results in this example.

### Run the ESP32 Side of the Example:
If you haven’t already, set up the ESP32 IDF framework:
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html

```
cd esp32-spi-message-demo/parse_meta/
idf.py build
idf.py -p /dev/ttyUSB1 flash
```

### Run the DepthAI Side of the Example:
`python3 main.py mobilenet-ssd.blob.sh8cmx8NCE1`
