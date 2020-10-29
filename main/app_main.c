/* SPI Slave example, sender (uses SPI master driver)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/igmp.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "soc/rtc_periph.h"
#include "esp32/rom/cache.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_spi_flash.h"

#include "driver/gpio.h"
#include "esp_intr_alloc.h"

#include "spi_messaging.h"
#include "spi_protocol.h"



/*
SPI sender (master) example.

This example is supposed to work together with the SPI receiver. It uses the standard SPI pins (MISO, MOSI, SCLK, CS) to 
transmit data over in a full-duplex fashion, that is, while the master puts data on the MOSI pin, the slave puts its own
data on the MISO pin.

This example uses one extra pin: GPIO_HANDSHAKE is used as a handshake pin. The slave makes this pin high as soon as it is
ready to receive/send data. This code connects this line to a GPIO interrupt which gives the rdySem semaphore. The main 
task waits for this semaphore to be given before queueing a transmission.
*/


/*
Pins in use. The SPI Master can use the GPIO mux, so feel free to change these if needed.
*/

// DepthAI uses SPI2.6 pin as an interrupt output, open-drain, active low.
#define GPIO_HANDSHAKE 2 // ESP32 pin that connects to SPI2.6

#if 0 // original config in esp-idf example
#define GPIO_MOSI 12
#define GPIO_MISO 13
#define GPIO_SCLK 15
#define GPIO_CS 14
#else // GPIOs used in the upcoming board: DepthAI with ESP-WROOM-32
#define GPIO_MOSI 13
#define GPIO_MISO 12
#define GPIO_SCLK 14
#define GPIO_CS 15
#endif

#define PAYLOAD_MAX_SIZE 252
#define BUFF_MAX_SIZE 256
#define SPI_PKT_SIZE 256

//The semaphore indicating the slave is ready to receive stuff.
static xQueueHandle rdySem;

static SpiProtocolPacket* emptyPacket;

/*
This ISR is called when the handshake line goes low.
*/
static void IRAM_ATTR gpio_handshake_isr_handler(void* arg)
{
    //Sometimes due to interference or ringing or something, we get two irqs after eachother. This is solved by
    //looking at the time between interrupts and refusing any interrupt too close to another one.
    static uint32_t lasthandshaketime;
    uint32_t currtime=xthal_get_ccount();
    uint32_t diff=currtime-lasthandshaketime;
    if (diff<240000) return; //ignore everything <1ms after an earlier irq
    lasthandshaketime=currtime;

    //Give the semaphore.
    BaseType_t mustYield=false;
    xSemaphoreGiveFromISR(rdySem, &mustYield);
    if (mustYield) portYIELD_FROM_ISR();
}

void debugPrintPacket(char * packet){
    for (int i = 0; i < 256; i++)
    {
        printf("%02X", packet[i]);
    }
    printf("\n\n");
}

uint8_t spi_get_size(SpiGetSizeResp *response, char * stream_name, SpiProtocolInstance* spiProtoInstance, SpiProtocolPacket* spiSendPacket, spi_transaction_t spi_trans, spi_device_handle_t handle){
    uint8_t success = 0;
    spi_send_command(spiSendPacket, GET_SIZE, strlen(stream_name)+1, stream_name);
    spi_trans.length=SPI_PKT_SIZE*8;
    spi_trans.tx_buffer=spiSendPacket;

    printf("sending GET_SIZE cmd.\n");
    spi_device_transmit(handle, &spi_trans);

    if(xSemaphoreTake(rdySem, ( TickType_t ) 500) == pdPASS){
        printf("receive GET_SIZE response from remote device...\n");
        char recvbuf[BUFF_MAX_SIZE] = {0};
        spi_trans.rx_buffer=recvbuf;
        spi_trans.tx_buffer=emptyPacket;
        spi_device_transmit(handle, &spi_trans);

        if(recvbuf[0]==0xaa){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spiProtoInstance, (uint8_t*)recvbuf, sizeof(recvbuf));
            spi_parse_get_size_resp(response, spiRecvPacket->data);
            success = 1;
            
        }else if(recvbuf[0] != 0x00){
            printf("*************************************** got a half/non aa packet ************************************************\n");
            success = 0;
        }
    } else {
        printf("Timeout: no response from remote device...\n");
        success = 0;
    }

    return success;
}


uint8_t spi_get_message(SpiGetMessageResp *response, char * stream_name, uint32_t size, SpiProtocolInstance* spiProtoInstance, SpiProtocolPacket* spiSendPacket, spi_transaction_t spi_trans, spi_device_handle_t handle){
    uint8_t success = 0;
int debug_skip = 0;
    spi_send_command(spiSendPacket, GET_MESSAGE, strlen(stream_name)+1, stream_name);
    spi_trans.length=SPI_PKT_SIZE*8;
    spi_trans.tx_buffer=spiSendPacket;

    printf("sending GET_MESSAGE cmd.\n");
    spi_device_transmit(handle, &spi_trans);

    uint32_t total_recv = 0;
    while(total_recv < size){
        if(xSemaphoreTake(rdySem, ( TickType_t ) 500) == pdPASS){

if(debug_skip%20 == 0){
    printf("receive GET_MESSAGE response from remote device... %d/%d\n", total_recv, size);
}
debug_skip++;

            char recvbuf[BUFF_MAX_SIZE] = {0};
            spi_trans.rx_buffer=recvbuf;
            spi_trans.tx_buffer=emptyPacket;
            spi_device_transmit(handle, &spi_trans);

//    for(int i = 0; i<BUFF_MAX_SIZE; i++){
//        printf("%02x", recvbuf[i]);
//    }
//    printf("\n");

                if(recvbuf[0]==0xaa){
                    SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spiProtoInstance, (uint8_t*)recvbuf, sizeof(recvbuf));
                    uint32_t remaining_data = size-total_recv;
                    if ( remaining_data < PAYLOAD_MAX_SIZE ){
                        memcpy(response->data+total_recv, spiRecvPacket->data, remaining_data);
                        total_recv += remaining_data;
                    } else {
                        memcpy(response->data+total_recv, spiRecvPacket->data, PAYLOAD_MAX_SIZE);
                        total_recv += PAYLOAD_MAX_SIZE;
                    }

                }else if(recvbuf[0] != 0x00){
                    printf("*************************************** got a half/non aa packet ************************************************\n");
                }
            success = 1;
        } else {
            printf("Timeout: no response from remote device...\n");
            success = 0;
            break;
        }
    }

/*
printf("asdfasdf %d %d\n", total_recv, size);
for (int i=0; i<total_recv; i++){
    if(i%80==0){
        printf("\n");
    }
    printf("%02x", response->data[i]);
}
printf("\n");
*/

    // grab the last 4 bytes and convert to 
    spi_parse_get_message(response, response->data, size);

    return success;
}

uint8_t spi_pop_messages(SpiPopMessagesResp *response, char * stream_name, SpiProtocolInstance* spiProtoInstance, SpiProtocolPacket* spiSendPacket, spi_transaction_t spi_trans, spi_device_handle_t handle){
    uint8_t success = 0;
    spi_send_command(spiSendPacket, POP_MESSAGES, strlen(stream_name)+1, stream_name);
    spi_trans.length=SPI_PKT_SIZE*8;
    spi_trans.tx_buffer=spiSendPacket;

    printf("sending POP_MESSAGES cmd.\n");
    spi_device_transmit(handle, &spi_trans);

    if(xSemaphoreTake(rdySem, ( TickType_t ) 500) == pdPASS){
        printf("receive POP_MESSAGES response from remote device...\n");
        char recvbuf[BUFF_MAX_SIZE] = {0};
        spi_trans.rx_buffer=recvbuf;
        spi_trans.tx_buffer=emptyPacket;
        spi_device_transmit(handle, &spi_trans);

        if(recvbuf[0]==0xaa){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spiProtoInstance, (uint8_t*)recvbuf, sizeof(recvbuf));
            spi_parse_pop_messages_resp(response, spiRecvPacket->data);
            success = 1;
            
        }else if(recvbuf[0] != 0x00){
            printf("asdfasdf *************************************** got a half/non aa packet ************************************************\n");
            success = 0;
        }
    } else {
        printf("Timeout: no response from remote device...\n");
        success = 0;
    }

    return success;
}

uint8_t spi_get_streams(SpiGetStreamsResp *response, SpiProtocolInstance* spiProtoInstance, SpiProtocolPacket* spiSendPacket, spi_transaction_t spi_trans, spi_device_handle_t handle){
    uint8_t success = 0;
    spi_send_command(spiSendPacket, GET_STREAMS, 1, "");
    spi_trans.length=SPI_PKT_SIZE*8;
    spi_trans.tx_buffer=spiSendPacket;

    printf("sending GET_STREAMS cmd.\n");
    spi_device_transmit(handle, &spi_trans);

    if(xSemaphoreTake(rdySem, ( TickType_t ) 500) == pdPASS){
        printf("receive GET_STREAMS response from remote device...\n");
        char recvbuf[BUFF_MAX_SIZE] = {0};
        spi_trans.rx_buffer=recvbuf;
        spi_trans.tx_buffer=emptyPacket;
        spi_device_transmit(handle, &spi_trans);

        if(recvbuf[0]==0xaa){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spiProtoInstance, (uint8_t*)recvbuf, sizeof(recvbuf));
            spi_parse_get_streams_resp(response, spiRecvPacket->data);
            success = 1;
            
        }else if(recvbuf[0] != 0x00){
            printf("asdfasdf *************************************** got a half/non aa packet ************************************************\n");
            success = 0;
        }
    } else {
        printf("Timeout: no response from remote device...\n");
        success = 0;
    }

    return success;
}

//Main application
void app_main()
{
    esp_err_t ret;
    spi_device_handle_t handle;
    uint8_t req_success = 0;

    emptyPacket = calloc(sizeof(SpiProtocolPacket), sizeof(uint32_t));

//--------------------------------------------------
// ------------------------------------------------
// testing spi_protocol.
// ------------------------------------------------

    SpiProtocolInstance* spiProtoInstance = malloc(sizeof(SpiProtocolInstance));
    SpiProtocolPacket* spiSendPacket = malloc(sizeof(SpiProtocolPacket));

    static char recvbuf[BUFF_MAX_SIZE] = {0};
    spi_transaction_t spi_trans;
    memset(&spi_trans, 0, sizeof(spi_trans));

//--------------------------------------------------

    //Configuration for the SPI bus
    spi_bus_config_t buscfg={
        .mosi_io_num=GPIO_MOSI,
        .miso_io_num=GPIO_MISO,
        .sclk_io_num=GPIO_SCLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };

    //Configuration for the SPI device on the other side of the bus
    spi_device_interface_config_t devcfg={
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .clock_speed_hz=4000000, // TODO
        .duty_cycle_pos=128,        //50% duty cycle
        .mode=0,
        .spics_io_num=GPIO_CS,
        .cs_ena_posttrans=3,        //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .queue_size=3
    };

    //GPIO config for the handshake line.
    gpio_config_t io_conf={
        .intr_type=GPIO_PIN_INTR_NEGEDGE,
        .mode=GPIO_MODE_INPUT,
        .pull_up_en=1,
        .pin_bit_mask=(1<<GPIO_HANDSHAKE)
    };






    //Create the semaphore.
    rdySem=xSemaphoreCreateBinary();

    //Set up handshake line interrupt.
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_set_intr_type(GPIO_HANDSHAKE, GPIO_PIN_INTR_NEGEDGE);
    gpio_isr_handler_add(GPIO_HANDSHAKE, gpio_handshake_isr_handler, NULL);

    //Initialize the SPI bus and add the device we want to send stuff to.
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &handle);
    assert(ret==ESP_OK);

    //Assume the slave is ready for the first transmission: if the slave started up before us, we will not detect 
    //positive edge on the handshake line.
    xSemaphoreGive(rdySem);




    // init spi protocol
    spi_protocol_init(spiProtoInstance);


    // take semaphore for first time.
    xSemaphoreTake(rdySem, ( TickType_t ) 50);

    // example of grabbing the available streams. they'll be the same as what's defined in the nodes.
    SpiGetStreamsResp p_get_streams_resp;
    req_success = spi_get_streams(&p_get_streams_resp, spiProtoInstance, spiSendPacket, spi_trans, handle);
    printf("Available Streams: \n");
    for(int i=0; i<p_get_streams_resp.numStreams; i++){
        printf("%s\n", p_get_streams_resp.stream_names[i]);
    }

    while(1) {
//usleep(50000);
        // do a get_size before trying to retreive message.
        SpiGetSizeResp p_get_size_resp;
        req_success = spi_get_size(&p_get_size_resp, "spimetaout", spiProtoInstance, spiSendPacket, spi_trans, handle);
        printf("response: %d\n", p_get_size_resp.size);

        // get message (assuming we got size)
        if(req_success){
            SpiGetMessageResp p_get_message_resp;
            p_get_message_resp.data = malloc(p_get_size_resp.size);
            req_success = spi_get_message(&p_get_message_resp, "spimetaout", p_get_size_resp.size, spiProtoInstance, spiSendPacket, spi_trans, handle);

            

            free(p_get_message_resp.data);
            if(req_success){
                SpiPopMessagesResp p_pop_messages_resp;
                req_success = spi_pop_messages(&p_pop_messages_resp, "spimetaout", spiProtoInstance, spiSendPacket, spi_trans, handle);
            }
        }
    }


    //Never reached.
    ret=spi_bus_remove_device(handle);
    assert(ret==ESP_OK);
}
