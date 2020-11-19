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

#include <nlohmann/json.hpp>

#include "spi_messaging.h"
#include "spi_protocol.h"
#include "driver/gpio.h"

#include "decode_mobilenet.h"
#include "esp32_spi_impl.h"

#define DEBUG_CMD 0
#define debug_cmd_print(...) \
    do { if (DEBUG_CMD) fprintf(stderr, __VA_ARGS__); } while (0)

#define DEBUG_METADATA 1

#define DEBUG_MESSAGE_CONTENTS 0
#define MAX_DETECTIONS 16

#define GPIO_OUTPUT_IO_0    GPIO_NUM_33

using json = nlohmann::json;

extern "C" {
   void app_main();
}

//---------------------------------------------------------------------
// You can create your own send and recv methods for different MCU and assign them to the function pointer here. 
// In theory that's all that's needed when switching to a different MCU. We'll see in practice.
//---------------------------------------------------------------------
uint8_t (*send_spi_ptr)(SpiProtocolPacket* spiSendPacket) = &esp32_send_spi;
uint8_t (*recv_spi_ptr)(char* recvbuf) = &esp32_recv_spi;

uint8_t generic_send_spi(SpiProtocolPacket* spiSendPacket){
    return (*send_spi_ptr)(spiSendPacket); 
}

uint8_t generic_recv_spi(char* recvbuf){
    return (*recv_spi_ptr)(recvbuf);
}
//---------------------------------------------------------------------

void debug_print_hex(uint8_t * data, int len){
    for(int i=0; i<len; i++){
        if(i%80==0){
            printf("\n");
        }
        printf("%02x", data[i]);
    }
    printf("\n");
}

void debug_print_char(char * data, int len){
    for(int i=0; i<len; i++){
        printf("%c", data[i]);
    }
    printf("\n");
}

void reset_mx(){
//    gpio_config_t io_conf;
//    io_conf.mode = GPIO_MODE_OUTPUT;
//    io_conf.pull_down_en = 0;
//    io_conf.pull_up_en = 0;
//    gpio_config(&io_conf);
    printf("in reset_mx\n");
    gpio_set_level(GPIO_OUTPUT_IO_0, 1);
    usleep(2000);
    gpio_set_level(GPIO_OUTPUT_IO_0, 0);
    usleep(2000);
}





uint8_t spi_get_size(SpiGetSizeResp *response, char * stream_name, SpiProtocolInstance* spiProtoInstance, SpiProtocolPacket* spiSendPacket){
    uint8_t success = 0;
    debug_cmd_print("sending GET_SIZE cmd.\n");
    spi_generate_command(spiSendPacket, GET_SIZE, strlen(stream_name)+1, stream_name);
    generic_send_spi(spiSendPacket);

    debug_cmd_print("receive GET_SIZE response from remote device...\n");
    char recvbuf[BUFF_MAX_SIZE] = {0};
    uint8_t recv_success = generic_recv_spi(recvbuf);

    if(recv_success){
        if(recvbuf[0]==START_BYTE_MAGIC){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spiProtoInstance, (uint8_t*)recvbuf, sizeof(recvbuf));
            spi_parse_get_size_resp(response, spiRecvPacket->data);
            success = 1;
            
        }else if(recvbuf[0] != 0x00){
            printf("*************************************** got a half/non aa packet ************************************************\n");
            success = 0;
        }
    } else {
        printf("failed to recv packet\n");
        success = 0;
    }

    return success;
}


uint8_t spi_get_message(SpiGetMessageResp *response, char * stream_name, uint32_t size, SpiProtocolInstance* spiProtoInstance, SpiProtocolPacket* spiSendPacket){
    uint8_t success = 0;
    debug_cmd_print("sending GET_MESSAGE cmd.\n");
    spi_generate_command(spiSendPacket, GET_MESSAGE, strlen(stream_name)+1, stream_name);
    generic_send_spi(spiSendPacket);

    uint32_t total_recv = 0;
    int debug_skip = 0;
    while(total_recv < size){
        if(debug_skip%20 == 0){
            debug_cmd_print("receive GET_MESSAGE response from remote device... %d/%d\n", total_recv, size);
        }
        debug_skip++;

        char recvbuf[BUFF_MAX_SIZE] = {0};
        uint8_t recv_success = generic_recv_spi(recvbuf);
        if(recv_success){
            if(recvbuf[0]==START_BYTE_MAGIC){
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
                break;
            }
        } else {
            printf("failed to recv packet\n");
            break;
        }
    }

    if(total_recv==size){
        spi_parse_get_message(response, response->data, size);

        if(DEBUG_MESSAGE_CONTENTS){
            printf("metadata_size: %d metadata_type: %d data_size: %d\n", response->metadata_size, response->metadata_type, response->data_size);
            debug_print_char((char*)response->metadata, response->metadata_size);
            debug_print_hex((uint8_t*)response->data, response->data_size);
        }

        success = 1;
    } else {
        success = 0;
    }

    return success;
}

uint8_t spi_pop_messages(SpiStatusResp *response, SpiProtocolInstance* spiProtoInstance, SpiProtocolPacket* spiSendPacket){
    uint8_t success = 0;

    debug_cmd_print("sending POP_MESSAGES cmd.\n");
    spi_generate_command(spiSendPacket, POP_MESSAGES, strlen("")+1, "");
    generic_send_spi(spiSendPacket);

    debug_cmd_print("receive POP_MESSAGES response from remote device...\n");
    char recvbuf[BUFF_MAX_SIZE] = {0};
    uint8_t recv_success = generic_recv_spi(recvbuf);

    if(recv_success){
        if(recvbuf[0]==START_BYTE_MAGIC){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spiProtoInstance, (uint8_t*)recvbuf, sizeof(recvbuf));
            spi_status_resp(response, spiRecvPacket->data);
            success = 1;
            
        }else if(recvbuf[0] != 0x00){
            printf("*************************************** got a half/non aa packet ************************************************\n");
            success = 0;
        }
    } else {
        printf("failed to recv packet\n");
        success = 0;
    }


    return success;
}

uint8_t spi_get_streams(SpiGetStreamsResp *response, SpiProtocolInstance* spiProtoInstance, SpiProtocolPacket* spiSendPacket){
    uint8_t success = 0;
    debug_cmd_print("sending GET_STREAMS cmd.\n");
    spi_generate_command(spiSendPacket, GET_STREAMS, 1, "");
    generic_send_spi(spiSendPacket);

    debug_cmd_print("receive GET_STREAMS response from remote device...\n");
    char recvbuf[BUFF_MAX_SIZE] = {0};
    uint8_t recv_success = generic_recv_spi(recvbuf);

    if(recv_success){
        if(recvbuf[0]==START_BYTE_MAGIC){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spiProtoInstance, (uint8_t*)recvbuf, sizeof(recvbuf));
            spi_parse_get_streams_resp(response, spiRecvPacket->data);
            success = 1;
            
        }else if(recvbuf[0] != 0x00){
            printf("*************************************** got a half/non aa packet ************************************************\n");
            success = 0;
        }
    } else {
        printf("failed to recv packet\n");
        success = 0;
    }

    return success;
}

uint8_t spi_pop_message(SpiStatusResp *response, char * stream_name, SpiProtocolInstance* spiProtoInstance, SpiProtocolPacket* spiSendPacket){
    uint8_t success = 0;

    debug_cmd_print("sending POP_MESSAGE cmd.\n");
    spi_generate_command(spiSendPacket, POP_MESSAGE, strlen(stream_name)+1, stream_name);
    generic_send_spi(spiSendPacket);

    debug_cmd_print("receive POP_MESSAGE response from remote device...\n");
    char recvbuf[BUFF_MAX_SIZE] = {0};
    uint8_t recv_success = generic_recv_spi(recvbuf);

    if(recv_success){
        if(recvbuf[0]==START_BYTE_MAGIC){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spiProtoInstance, (uint8_t*)recvbuf, sizeof(recvbuf));
            spi_status_resp(response, spiRecvPacket->data);
            success = 1;
            
        }else if(recvbuf[0] != 0x00){
            printf("*************************************** got a half/non aa packet ************************************************\n");
            success = 0;
        }
    } else {
        printf("failed to recv packet\n");
        success = 0;
    }


    return success;
}

//Main application
void app_main()
{
    uint8_t req_success = 0;
    json j;

    //TODO: move these into spi messaging?
    SpiProtocolInstance* spiProtoInstance = (SpiProtocolInstance*) malloc(sizeof(SpiProtocolInstance));
    SpiProtocolPacket* spiSendPacket = (SpiProtocolPacket*) malloc(sizeof(SpiProtocolPacket));

    // init spi for the esp32
    init_esp32_spi();

    // init spi protocol
    spi_protocol_init(spiProtoInstance);

    // example of grabbing the available streams. they'll be the same as what's defined in the nodes.
    SpiGetStreamsResp p_get_streams_resp;
    req_success = spi_get_streams(&p_get_streams_resp, spiProtoInstance, spiSendPacket);
    printf("Available Streams: \n");
    for(int i=0; i<p_get_streams_resp.numStreams; i++){
        printf("%s\n", p_get_streams_resp.stream_names[i]);
    }

    int cnt = 0;
    while(1) {
        // do a get_size before trying to retreive message.
        SpiGetSizeResp p_get_size_resp;
        req_success = spi_get_size(&p_get_size_resp, "spimetaout", spiProtoInstance, spiSendPacket);
        debug_cmd_print("response: %d\n", p_get_size_resp.size);

        // get message (assuming we got size)
        if(req_success){
            SpiGetMessageResp p_get_message_resp;
            p_get_message_resp.data = (uint8_t*) malloc(p_get_size_resp.size);
            if(!p_get_message_resp.data){
                printf("total free %d\n", esp_get_free_heap_size());
                printf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
                printf("Failed to allocate memory for response of size %d.\n", p_get_size_resp.size);
                assert(0);
            }

            req_success = spi_get_message(&p_get_message_resp, "spimetaout", p_get_size_resp.size, spiProtoInstance, spiSendPacket);

            // ----------------------------------------
            // example of decoding mobilenet
            // ----------------------------------------
            if(req_success){
                Detection dets[MAX_DETECTIONS];

                int num_found = decode_mobilenet(dets, (half *)p_get_message_resp.data, 0.5f, MAX_DETECTIONS);
                printf("num_found %d \n", num_found);

                if(num_found > 0){
                    for(int i=0; i<num_found; i++){
                        printf("LABEL:%f X(%.3f %.3f), Y(%.3f %.3f) CONFIDENCE: %.3f\n",
                                dets[i].label,
                                dets[i].x_min, dets[i].x_max,
                                dets[i].y_min, dets[i].y_max, dets[i].confidence);
                    }
                }else{
                    printf("none found\n");
                }

                if(DEBUG_METADATA){
                    j = json::from_msgpack(p_get_message_resp.metadata, p_get_message_resp.metadata + p_get_message_resp.metadata_size);
                    printf("Unpacked metadata: %s\n", j.dump().c_str());
                }
            }

            // ----------------------------------------

            free(p_get_message_resp.data);

            if(req_success){
                SpiStatusResp p_status_resp;
//                req_success = spi_pop_messages(&p_status_resp, spiProtoInstance, spiSendPacket);
                req_success = spi_pop_message(&p_status_resp, "spimetaout", spiProtoInstance, spiSendPacket);

            }
        }

//        cnt++;
//        if(cnt%5 == 0){
//            reset_mx();
//        }
    }


    //Never reached.
    deinit_esp32_spi();

}
