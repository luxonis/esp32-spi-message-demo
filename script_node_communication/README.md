## 2-way communication between Script node and ESP32

ESP32 sends `Hello` string to the VPU (Script node), where ` World` string is added to initial string and the concatenated string is then sent back to the ESP32, which prints `Hello World` to the user.

Instead of `Hello World` string, you could send different messages like some values or custom commands to perform some action on either VPU or ESP32.

### On the DepthAI:
In main.py, we create a pipeline that waits for a Buffer message from SPIIn. After Buffer is recieved, it's serialized (ASCII encoding), it adds ` World` to the end of the text and sends the ASCII string back (with Buffer message) to the SPIOut (to ESP32).

### On the ESP32:
ESP32 creates Buffer message and sets `Hello` string to it, then sends the Buffer to the VPU. It waits for the response, serializes the response and prints it to the user.

### Requirements:
This demo requires an ESP32 board connected via SPI to the DepthAI. The easiest way to accomplish this is to get a hold of an [OAK IOT device](https://docs.luxonis.com/projects/hardware/en/latest/#iot-designs), as they have an integrated ESP32 already connected to the Myriad X VPU via SPI.

### Run the ESP32 Side of the Example:
If you haven’t already, set up the ESP32 IDF framework:
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html

```
cd ../esp32-spi-message-demo/script_node_communication/
idf.py build
# Use the correct path here - it might not be ttyUSB0
idf.py -p /dev/ttyUSB0 flash
```

### Run the DepthAI Side of the Example:
`python3 main.py`
