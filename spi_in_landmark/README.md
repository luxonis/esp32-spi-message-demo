## Gen2 SPIIn/SPIOut Loopback

### Overview:
This demo requires an ESP32 board connected via SPI to the DepthAI. The easiest way to accomplish this is to get a hold of an BW1092 board. It has an integrated ESP32 already connected to the DepthAI.

### On the DepthAI:
In main.py, we create a pipeline that just loops back input from SPIIn to SPIOut.

### On the ESP32:
The ESP32 is generating a fake 180x180 image to send back to SPIIn. After sending it waits for a response back from the depthai containing the same image.

### Run the ESP32 Side of the Example:
If you haven’t already, set up the ESP32 IDF framework:
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html

```
cd ../esp32-spi-message-demo/spi_send_demo/
idf.py build
idf.py -p /dev/ttyUSB1 flash
```

### Run the DepthAI Side of the Example:
`python3 main.py`
