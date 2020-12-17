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
git submodule update --init --recursive
```

### Download depthai-core, which contains the pipeline builder example
```
cd ~
git clone https://github.com/luxonis/depthai-core.git
cd depthai-core
git checkout gen2_spi
git submodule update --init --recursive
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

## Running Tiny-YOLO, Including Standalone

### Flashing a tinyYOLO Image
1. From the depthai github repo, use this command to build a tiny yolo blob. Additional testing required to see if we can increase the number of shaves from 4. 

`./depthai_demo.py -cnn tiny-yolo-v3 -sh 4 -cmx 4 -nce 1`

2. If using your own neural network, update `tiny-yolo-v3.json` with your graph’s info. If you have a non 416x416 input size, add the `in_height` and `in_width` as below (done for an example of 624x624 input resolution):
`vim resources/nn/tiny-yolo-v3/tiny-yolo.json`

```
"NN_config":
    {
        "output_format" : "detection",
        "NN_family" : "YOLO",
        "NN_specific_metadata" :
        { 
            "classes" : 80,
            "coordinates" : 4,
            "in_height" : 624,
            "in_width" : 624,
            "anchors" : [10,14, 23,27, 37,58, 81,82, 135,169, 344,319],
            "anchor_masks" : 
            {
                "side26" : [1,2,3],
                "side13" : [3,4,5]
            },
            "iou_threshold" : 0.5,
            "confidence_threshold" : 0.5
        }
    },
    
```

3. Change directories to the standaloneYOLOExample directory in your depthai-core repo.

`cd <mypath>/depthai-python/depthai-core/standaloneYOLOExample`

4. Checkout the gen2_common_objdet branch. ***Will be merged into the gen2 branch in the future.
`git checkout gen2_common_objdet`

5. Build the example.

`cmake .`
`cmake --build .`

6. Make sure you’ve already installed the bootloader as instructed here and set the device to flash boot mode:

https://docs.google.com/document/d/1Q0Wwjs0djMQOPwRT04k8tL20WWv_5AdwiQcPSeebqsw/edit

7. Run the example and wait for the image to flash..

`./myapp <mypath>/depthai/resources/nn/tiny-yolo-v3/tiny-yolo-v3.blob.sh4cmx4NCE1 <mypath>/depthai/resources/nn/tiny-yolo-v3/tiny-yolo-v3.json`

Make sure the USB C cable is not attached to your DepthAI and reboot the board.

### ESP32 Setup
For the ESP32 side code, please take a look at the latest on the main branch. You’ll need a git submodule init and update. You’ll also want to disable the this flag in app_main.cpp:

`#define DECODE_MOBILENET 1`

# Known Issues
* STABILITY - Although we've caught and fixed a bunch of bugs, and this now seems to be stable, there could still be some lurking ones, and this codebase is under constant improvement, which could include breaking SPI-API changes.  The codebase should be considered transitioning from Alpha to Beta.
* 300x300 RGB preview is just a little too big to fit in ESP32 memory. Create an example of grabbing MJPEG instead.
