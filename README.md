
## NOTE!

OAK IoT series and this repository is **community supported only**, and is **provided as-is**. We most likely **won't update it** and **we don't provide support** for it (Discord, forums, email...).

# Demo

See below for this running on the [BW1092](https://shop.luxonis.com/collections/all/products/bw1092):

[![SPI ESP32 Interface with DepthAI](https://user-images.githubusercontent.com/32992551/102835329-9bfa2100-43b3-11eb-8cce-ce65cb8e600d.png)](https://www.youtube.com/watch?v=S2xYcVoyPxk "Embedded UseCase of DepthAI")

## Building

The first time you build, the repository submodules need be initialized:
```
git submodule update --init --recursive

# Tip: You can ask Git to do that automatically:
git config submodule.recurse true
```

Later submodules also need to be updated. To build an example, you will need to use [ESP-IDF](https://www.espressif.com/en/products/sdks/esp-idf)'s `idf.py`. **Examples here were only tested with ESP-IDF version 4.1** and we encourage you to use the same version as well.

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
