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

#include "SpiPacketParser.hpp"
#include "depthai-shared/datatype/RawImgDetections.hpp"

#include <memory>

#define DECODE_MOBILENET 1

#define DEBUG_CMD 0
#define debug_cmd_print(...) \
    do { if (DEBUG_CMD) fprintf(stderr, __VA_ARGS__); } while (0)

#define DEBUG_METADATA 0

#define DEBUG_MESSAGE_CONTENTS 0
#define MAX_DETECTIONS 16

#define GPIO_OUTPUT_IO_0    GPIO_NUM_33

using json = nlohmann::json;

static const char* NOSTREAM = "";
static const char* METASTREAM = "spimetaout";
static const char* PREVIEWSTREAM = "spipreview";

extern "C" {
   void app_main();
}

//---------------------------------------------------------------------
// You can create your own send and recv methods for different MCU and assign them to the function pointer here. 
// In theory that's all that's needed when switching to a different MCU. We'll see in practice.
//---------------------------------------------------------------------
uint8_t (*send_spi_ptr)(SpiProtocolPacket* spi_send_packet) = &esp32_send_spi;
uint8_t (*recv_spi_ptr)(char* recvbuf) = &esp32_recv_spi;

uint8_t generic_send_spi(SpiProtocolPacket* spi_send_packet){
    return (*send_spi_ptr)(spi_send_packet); 
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

void alloc_fail(uint32_t size){
    printf("total free %d\n", esp_get_free_heap_size());
    printf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    printf("Failed to allocate memory for response of size %d.\n", size);
    assert(0);
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




// ----------------------------------------
// example of individual spi messaging cmds
// ----------------------------------------
uint8_t spi_get_size(SpiGetSizeResp *response, spi_command get_size_cmd, const char * stream_name, SpiProtocolInstance* spi_proto_instance, SpiProtocolPacket* spi_send_packet){
    assert(isGetSizeCmd(get_size_cmd));

    uint8_t success = 0;
    debug_cmd_print("sending spi_get_size cmd.\n");
    spi_generate_command(spi_send_packet, get_size_cmd, strlen(stream_name)+1, stream_name);
    generic_send_spi(spi_send_packet);

    debug_cmd_print("receive spi_get_size response from remote device...\n");
    char recvbuf[BUFF_MAX_SIZE] = {0};
    uint8_t recv_success = generic_recv_spi(recvbuf);

    if(recv_success){
        if(recvbuf[0]==START_BYTE_MAGIC){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spi_proto_instance, (uint8_t*)recvbuf, sizeof(recvbuf));
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

uint8_t spi_get_message(SpiGetMessageResp *response, spi_command get_mess_cmd, const char * stream_name, uint32_t size, SpiProtocolInstance* spi_proto_instance, SpiProtocolPacket* spi_send_packet){
    assert(isGetMessageCmd(get_mess_cmd));

    uint8_t success = 0;
    debug_cmd_print("sending spi_get_message cmd.\n");
    spi_generate_command(spi_send_packet, get_mess_cmd, strlen(stream_name)+1, stream_name);
    generic_send_spi(spi_send_packet);

    uint32_t total_recv = 0;
    int debug_skip = 0;
    while(total_recv < size){
        if(debug_skip%20 == 0){
            debug_cmd_print("receive spi_get_message response from remote device... %d/%d\n", total_recv, size);
        }
        debug_skip++;

        char recvbuf[BUFF_MAX_SIZE] = {0};
        uint8_t recv_success = generic_recv_spi(recvbuf);
        if(recv_success){
            if(recvbuf[0]==START_BYTE_MAGIC){
                SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spi_proto_instance, (uint8_t*)recvbuf, sizeof(recvbuf));
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
        spi_parse_get_message(response, size, get_mess_cmd);

        if(DEBUG_MESSAGE_CONTENTS){
            printf("data_size: %d\n", response->data_size);
            debug_print_hex((uint8_t*)response->data, response->data_size);
        }
        success = 1;
    } else {
        success = 0;
    }

    return success;
}

uint8_t spi_pop_messages(SpiStatusResp *response, SpiProtocolInstance* spi_proto_instance, SpiProtocolPacket* spi_send_packet){
    uint8_t success = 0;

    debug_cmd_print("sending POP_MESSAGES cmd.\n");
    spi_generate_command(spi_send_packet, POP_MESSAGES, strlen(NOSTREAM)+1, NOSTREAM);
    generic_send_spi(spi_send_packet);

    debug_cmd_print("receive POP_MESSAGES response from remote device...\n");
    char recvbuf[BUFF_MAX_SIZE] = {0};
    uint8_t recv_success = generic_recv_spi(recvbuf);

    if(recv_success){
        if(recvbuf[0]==START_BYTE_MAGIC){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spi_proto_instance, (uint8_t*)recvbuf, sizeof(recvbuf));
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

uint8_t spi_get_streams(SpiGetStreamsResp *response, SpiProtocolInstance* spi_proto_instance, SpiProtocolPacket* spi_send_packet){
    uint8_t success = 0;
    debug_cmd_print("sending GET_STREAMS cmd.\n");
    spi_generate_command(spi_send_packet, GET_STREAMS, 1, NOSTREAM);
    generic_send_spi(spi_send_packet);

    debug_cmd_print("receive GET_STREAMS response from remote device...\n");
    char recvbuf[BUFF_MAX_SIZE] = {0};
    uint8_t recv_success = generic_recv_spi(recvbuf);

    if(recv_success){
        if(recvbuf[0]==START_BYTE_MAGIC){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spi_proto_instance, (uint8_t*)recvbuf, sizeof(recvbuf));
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

uint8_t spi_pop_message(SpiStatusResp *response, const char * stream_name, SpiProtocolInstance* spi_proto_instance, SpiProtocolPacket* spi_send_packet){
    uint8_t success = 0;

    debug_cmd_print("sending POP_MESSAGE cmd.\n");
    spi_generate_command(spi_send_packet, POP_MESSAGE, strlen(stream_name)+1, stream_name);
    generic_send_spi(spi_send_packet);

    debug_cmd_print("receive POP_MESSAGE response from remote device...\n");
    char recvbuf[BUFF_MAX_SIZE] = {0};
    uint8_t recv_success = generic_recv_spi(recvbuf);

    if(recv_success){
        if(recvbuf[0]==START_BYTE_MAGIC){
            SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spi_proto_instance, (uint8_t*)recvbuf, sizeof(recvbuf));
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
// ----------------------------------------


// ----------------------------------------
// example of decoding mobilenet
// ----------------------------------------
void exampleDecodeMobilenet(SpiGetMessageResp get_message_resp){
    json j;

    Detection dets[MAX_DETECTIONS];

    int num_found = decode_mobilenet(dets, (half *)get_message_resp.data, 0.5f, MAX_DETECTIONS);
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
}
// ----------------------------------------


// ----------------------------------------
// example of getting message data
// ----------------------------------------
uint8_t req_data(SpiGetMessageResp *get_message_resp, const char* stream_name, SpiProtocolInstance* spi_proto_instance, SpiProtocolPacket* spi_send_packet){
    uint8_t req_success = 0;

    // do a get_size before trying to retreive message.
    SpiGetSizeResp get_size_resp;
    req_success = spi_get_size(&get_size_resp, GET_SIZE, stream_name, spi_proto_instance, spi_send_packet);
    debug_cmd_print("response: %d\n", get_size_resp.size);

    // get message (assuming we got size)
    if(req_success){
        get_message_resp->data = (uint8_t*) malloc(get_size_resp.size);
        if(!get_message_resp->data){
            alloc_fail(get_size_resp.size);
        }

        req_success = spi_get_message(get_message_resp, GET_MESSAGE, stream_name, get_size_resp.size, spi_proto_instance, spi_send_packet);
    }

    return req_success;
}
// ----------------------------------------


// ----------------------------------------
// example of getting message metadata
// ----------------------------------------
uint8_t req_metadata(SpiGetMessageResp *get_message_resp, const char* stream_name, SpiProtocolInstance* spi_proto_instance, SpiProtocolPacket* spi_send_packet){
    uint8_t req_success = 0;

    // do a get_size before trying to retreive message.
    SpiGetSizeResp get_size_resp;
    req_success = spi_get_size(&get_size_resp, GET_METASIZE, stream_name, spi_proto_instance, spi_send_packet);
    debug_cmd_print("response: %d\n", get_size_resp.size);

    // get message (assuming we got size)
    if(req_success){
        get_message_resp->data = (uint8_t*) malloc(get_size_resp.size);
        if(!get_message_resp->data){
            alloc_fail(get_size_resp.size);
        }

        req_success = spi_get_message(get_message_resp, GET_METADATA, stream_name, get_size_resp.size, spi_proto_instance, spi_send_packet);
    }

    return req_success;
}
// ----------------------------------------


// ----------------------------------------
// example of resending a large message as it's received
// ----------------------------------------
uint8_t resend_large_message(const char* stream_name, SpiProtocolInstance* spi_proto_instance, SpiProtocolPacket* spi_send_packet){
    uint8_t req_success = 0;

    // do a get_size before trying to retreive message.
    SpiGetSizeResp get_size_resp;
    req_success = spi_get_size(&get_size_resp, GET_SIZE, stream_name, spi_proto_instance, spi_send_packet);
    debug_cmd_print("response: %d\n", get_size_resp.size);

    if(req_success){
        // send a get message command (assuming we got size)
        spi_generate_command(spi_send_packet, GET_MESSAGE, strlen(stream_name)+1, stream_name);
        generic_send_spi(spi_send_packet);

        uint32_t size = get_size_resp.size;
        uint32_t total_recv = 0;
        while(total_recv < size){
            char recvbuf[BUFF_MAX_SIZE] = {0};
            req_success = generic_recv_spi(recvbuf);
            if(req_success){
                if(recvbuf[0]==START_BYTE_MAGIC){
                    SpiProtocolPacket* spiRecvPacket = spi_protocol_parse(spi_proto_instance, (uint8_t*)recvbuf, sizeof(recvbuf));
                    uint32_t remaining_data = size-total_recv;
                    if ( remaining_data < PAYLOAD_MAX_SIZE ){
                        //--------------------------------------------
                        //  Transmit data here
                        //--------------------------------------------
                        usleep(2000);
                        debug_cmd_print("mock transmit data... %d/%d\n", total_recv, size);
                        if(DEBUG_MESSAGE_CONTENTS){
                            debug_print_hex((uint8_t*)spiRecvPacket->data, PAYLOAD_MAX_SIZE);
                        }
                        
                        total_recv += remaining_data;
                    } else {
                        //--------------------------------------------
                        //  Transmit data here
                        //--------------------------------------------
                        usleep(2000);
                        debug_cmd_print("mock transmit data... %d/%d\n", total_recv, size);
                        if(DEBUG_MESSAGE_CONTENTS){
                            debug_print_hex((uint8_t*)spiRecvPacket->data, PAYLOAD_MAX_SIZE);
                        }
                        total_recv += PAYLOAD_MAX_SIZE;
                    }

                }else if(recvbuf[0] != 0x00){
                    printf("*************************************** got a half/non aa packet ************************************************\n");
                    req_success = 0;
                    break;
                }
            } else {
                printf("failed to recv packet\n");
                req_success = 0;
                break;
            }
        }
    }

    return req_success;
}
// ----------------------------------------




//Main application
void app_main()
{
    uint8_t req_success = 0;
    SpiProtocolInstance* spi_proto_instance = (SpiProtocolInstance*) malloc(sizeof(SpiProtocolInstance));
    SpiProtocolPacket* spi_send_packet = (SpiProtocolPacket*) malloc(sizeof(SpiProtocolPacket));

    // init spi for the esp32
    init_esp32_spi();

    // init spi protocol
    spi_protocol_init(spi_proto_instance);

    // ----------------------------------------
    // example of grabbing the available streams. they'll match what's defined in the SPI nodes.
    // ----------------------------------------
    SpiGetStreamsResp p_get_streams_resp;
    req_success = spi_get_streams(&p_get_streams_resp, spi_proto_instance, spi_send_packet);
    printf("Available Streams: \n");
    for(int i=0; i<p_get_streams_resp.numStreams; i++){
        printf("%s\n", p_get_streams_resp.stream_names[i]);
    }


    while(1) {
        // ----------------------------------------
        // example of receiving messages.
        // ----------------------------------------
        // the req_data method allocates memory for the received packet. we need to be sure to free it when we're done with it.
        SpiGetMessageResp get_message_resp;

        req_success = req_data(&get_message_resp, METASTREAM, spi_proto_instance, spi_send_packet);
        std::vector<uint8_t> data;
        if(req_success){
            if(DECODE_MOBILENET){
                // ----------------------------------------
                // example of decoding mobilenet (ENABLE DECODE_MOBILENET flag).
                // ----------------------------------------
                exampleDecodeMobilenet(get_message_resp);
            } else {
                // ----------------------------------------
                // receive raw data 
                // ----------------------------------------
                data = std::vector<std::uint8_t>(get_message_resp.data, get_message_resp.data + get_message_resp.data_size);    
            }
            free(get_message_resp.data);
        }

        // ----------------------------------------
        // example of getting message metadata
        // ----------------------------------------
        // the req_metadata method allocates memory for the received packet. we need to be sure to free it when we're done with it.
        SpiGetMessageResp get_meta_resp;

        req_success = req_metadata(&get_meta_resp, METASTREAM, spi_proto_instance, spi_send_packet);
        if(req_success){
            if(DEBUG_METADATA){
                json j = json::from_msgpack(get_meta_resp.data, get_meta_resp.data + get_meta_resp.data_size);
                printf("Unpacked metadata: %s\n", j.dump().c_str());
            }

            // ----------------------------------------
            // example of parsing out basic ImgDetection type.
            // ----------------------------------------
            switch ((dai::DatatypeEnum) get_meta_resp.data_type)
            {
            case dai::DatatypeEnum::ImgDetections :
            {
                dai::RawImgDetections det;
                dai::parseMessage(get_meta_resp.data, get_meta_resp.data_size, det);

                for(const auto& det : det.detections){
                    printf("label: %d, xmin: %f, ymin: %f, xmax: %f, ymax: %f\n", det.label, det.xmin, det.ymin, det.xmax, det.ymax);
                }
            }
            break;
            
            default:
                break;
            }
                    
            free(get_meta_resp.data);
        }


        // ----------------------------------------
        // example of retransmitting a jpeg
        // ----------------------------------------
        // get jpeg size
        //req_success = resend_large_message(PREVIEWSTREAM, spi_proto_instance, spi_send_packet);

        // ----------------------------------------
        // pop current message/metadata. this tells the depthai to update the info being passed back using the spi_cmds.
        // ----------------------------------------
        SpiStatusResp p_status_resp;
        req_success = spi_pop_messages(&p_status_resp, spi_proto_instance, spi_send_packet);
        // or you can pop them individually, like this:
        //req_success = spi_pop_message(&p_status_resp, METASTREAM, spi_proto_instance, spi_send_packet);
        //req_success = spi_pop_message(&p_status_resp, PREVIEWSTREAM, spi_proto_instance, spi_send_packet);
    }


    //Never reached.
    deinit_esp32_spi();
    free(spi_proto_instance);
    free(spi_send_packet);
}
