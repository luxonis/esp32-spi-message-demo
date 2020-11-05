# SPI Protocol

SPI messaging is currently arranged in 2 layers. The first is the spi protocol. The spi protocol is the lowest level. It defines a standard packet for all SPI communication. It is a 256 byte packet arranged in the following manner:

```
typedef struct {
    uint8_t start;
    uint8_t data[SPI_PROTOCOL_PAYLOAD_SIZE];
    uint8_t crc[2];
    uint8_t end;
} SpiProtocolPacket;
```
start and end are constant bytes to mark the beginning and end of packets.
```
static const uint8_t START_BYTE_MAGIC = 0b10101010;
static const uint8_t END_BYTE_MAGIC = 0b00000000;
```
# SPI Messaging
On top of this we have a layer called SPI messaging. This code defines the following:
* A list of a supported commands,
* A way to encapsulate commands going to the MyriadX over SPI.
* A way to receive and parse command responses. 
* We’ll go into greater depth as to how exactly to use this in the SPI Messaging Example.

# The SPI Messaging Example
In order to better show how to use SPI messaging we’ve created a basic example that essentially runs mobilenet on the color camera of DepthAI, then returns the NN output through SPI.  

First, let’s look at the nodes we’re setting up on the Host/DepthAI side. The host side pipeline builder code can be found here:
https://github.com/luxonis/depthai-core/tree/gen2_spi
Be sure you’re looking at the gen2_spi branch for this example.

The example nodes are assembled in the createNNPipelineSPI method of depthai-core/example/main.cpp. It should end up looking something like this: 

```
    dai::Pipeline p;

    // set up NN node
    auto nn1 = p.create<dai::node::NeuralNetwork>();
    nn1->setBlobPath(nnPath);

    // set up color camera and link to NN node
    auto colorCam = p.create<dai::node::ColorCamera>();
    colorCam->setPreviewSize(300, 300);
    colorCam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    colorCam->setInterleaved(false);
    colorCam->setCamId(0);
    colorCam->setColorOrder(dai::ColorCameraProperties::ColorOrder::BGR);
    colorCam->preview.link(nn1->input);

    // set up SPI out node and link to nn1
    auto spiOut = p.create<dai::node::SPIOut>();
    spiOut->setStreamName("spimetaout");
    spiOut->setBusId(0);
    nn1->out.link(spiOut->input);

    return p;
```



Next, let’s look at what the ESP32 device will be doing. The code for this can be found in it’s own repo here:
https://github.com/luxonis/esp32-spi-message-demo

The core logic for the ESP32 can be found in the app_main method in esp32-spi-messaging-demo/main/app_main.cpp. Here’s a high level overview of what it’s doing:
1. The ESP32 requests a list of available SPI streamNames using spi_get_streams. This step is optional as we created the pipeline nodes and already know the names of them.
2. The ESP32 sends a spi_get_size command for a specific streamName.
3. The ESP32 sends a spi_get_message command specifying a specific streamName
4. DepthAI will respond with data from that streamName. The ESP32 should expect to receive the amount of data specified in the spi_get_size command. Metadata is included at the end of this message as a json string.
5. Once you’re done with the current data, use the spi_pop_messages command to prepare a new set of data for retrieval from the MyriadX. The spi_pop_messages command currently pops data for all SPI nodes at once on the MyriadX.

# Building and Running
Note that these instructions are intended for a Linux box. It hasn’t been tested with Windows yet.

### Download the ESP32 demo repo
```
cd ~
git clone https://github.com/luxonis/esp32-spi-message-demo.git
cd esp32-spi-message-demo
git submodule init
git submodule update
```

### Download depthai-core, which contains the pipeline builder example
```
cd ~
git clone https://github.com/luxonis/depthai-core.git
cd depthai-core
git checkout gen2_spi
git submodule init
git submodule update
```

### If you haven’t already, set up the ESP32 programmer
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html

## Build and run the ESP32 side code. Make sure your ESP32 is attached to your PC. 
```
cd ~/esp32-spi-message-demo/
idf.py build
idf.py -p /dev/ttyUSB1 flash
```

### Build and run the pipeline builder binary. Make sure your DepthAI is attached to your PC.
```
cd ~/depthai-core/example
cmake .
cmake --build .
./myapp mobilenet-ssd/mobilenet-ssd.blob
```
At this point, you can start something up to monitor debug prints such as “cu” or “minicom”. The example should generate debug prints as objects are detected by the color camera. 

# Known Issues
* STABILITY - this is what I’d consider an alpha release. I’ve run into issues running long periods of time. We are actively working on this right now however testing such things take a little bit of time. 
* 300x300 RGB preview is just a little too big to fit in ESP32 memory. Create an example of grabbing MJPEG instead.
