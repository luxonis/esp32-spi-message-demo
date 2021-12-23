## Inference a frame over SPI

### Overview:
This demo requires an ESP32 board connected via SPI to the DepthAI. The easiest way to accomplish this is to get a hold of an [OAK IOT device](https://docs.luxonis.com/projects/hardware/en/latest/#iot-designs), as they have an integrated ESP32 already connected to the Myriad X VPU via SPI.

### On the DepthAI:
In main.py, we create a pipeline that runs inference on frames recieved from [SPIIn](https://docs.luxonis.com/projects/api/en/latest/components/nodes/spi_in/) and returns NN results to [SPIOut](https://docs.luxonis.com/projects/api/en/latest/components/nodes/spi_out/).

### On the ESP32:
The ESP32 is generating a fake 180x180 image to send back to SPIIn. After sending the frame, it waits for a response back from the OAK device containing the NN inference result; [NNData message](https://docs.luxonis.com/projects/api/en/latest/components/messages/nn_data/).

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
