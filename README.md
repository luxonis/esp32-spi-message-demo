# Demo

See below for this running on the [BW1092](https://shop.luxonis.com/collections/all/products/bw1092):

[![SPI ESP32 Interface with DepthAI](https://user-images.githubusercontent.com/32992551/102835329-9bfa2100-43b3-11eb-8cce-ce65cb8e600d.png)](https://www.youtube.com/watch?v=S2xYcVoyPxk "Embedded UseCase of DepthAI")

And see https://github.com/luxonis/depthai-experiments/tree/master/gen2-spi for pulling off JPEG and/or depth (including cropping on DepthAI) instead of the MobileNetv2-SSD or tinyYOLOv3 metadata shown below.

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

# The SPI Messaging Examples
We have a few SPI messaging examples in the depthai-experiments repository. Please see the following:
https://github.com/luxonis/depthai-experiments/tree/master/gen2-spi
