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

#include "decode_raw_mobilenet.h"
#include "esp32_spi_impl.h"


#include "spi_api.hpp"
#include "SpiPacketParser.hpp"
#include "depthai-shared/datatype/RawImgDetections.hpp"

#include <memory>


#define MAX_DETECTIONS 16

#define GPIO_OUTPUT_IO_0    GPIO_NUM_33

using json = nlohmann::json;

static const char* METASTREAM = "spimetaout";
static const char* PREVIEWSTREAM = "spipreview";

extern "C" {
   void app_main();
}

/*
void alloc_fail(uint32_t size){
    printf("total free %d\n", esp_get_free_heap_size());
    printf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    printf("Failed to allocate memory for response of size %d.\n", size);
    assert(0);
}
*/


// ----------------------------------------
// example of decoding raw mobilenet output
// ----------------------------------------
void exampleDecodeRawMobilenet(SpiGetMessageResp get_message_resp){
    json j;

    Detection dets[MAX_DETECTIONS];

    int num_found = decode_raw_mobilenet(dets, (half *)get_message_resp.data, 0.5f, MAX_DETECTIONS);
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
// example of resending a large message as it's received
// ----------------------------------------
/*
uint8_t resend_large_message(const char* stream_name, SpiProtocolInstance* spi_proto_instance, SpiProtocolPacket* spi_send_packet){
    uint8_t req_success = 0;

    // do a get_size before trying to retreive message.
    SpiGetSizeResp get_size_resp;
    req_success = spi_get_size(&get_size_resp, GET_SIZE, stream_name);
    debug_cmd_print("response: %d\n", get_size_resp.size);

    if(req_success){
        // send a get message command (assuming we got size)
        spi_generate_command(spi_send_packet, GET_MESSAGE, strlen(stream_name)+1, stream_name);
        generic_send_spi((char *)spi_send_packet);

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
*/
// ----------------------------------------


void runDemo(){
    uint8_t req_success = 0;

    SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);


    // ----------------------------------------
    // example of grabbing the available streams. they'll match what's defined in the SPI nodes.
    // ----------------------------------------
    SpiGetStreamsResp p_get_streams_resp;
    req_success = mySpiApi.spi_get_streams(&p_get_streams_resp);
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

        req_success = mySpiApi.req_data(&get_message_resp, METASTREAM);
        std::vector<uint8_t> data;
        if(req_success){
            if(DECODE_MOBILENET){
                // ----------------------------------------
                // example of decoding mobilenet (ENABLE DECODE_MOBILENET flag).
                // ----------------------------------------
                exampleDecodeRawMobilenet(get_message_resp);
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

        req_success = mySpiApi.req_metadata(&get_meta_resp, METASTREAM);
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
        //req_success = resend_large_message(PREVIEWSTREAM);

        // ----------------------------------------
        // pop current message/metadata. this tells the depthai to update the info being passed back using the spi_cmds.
        // ----------------------------------------
        req_success = mySpiApi.spi_pop_messages();
        // or you can pop them individually, like this:
        //req_success = spi_pop_message(&p_status_resp, METASTREAM);
        //req_success = spi_pop_message(&p_status_resp, PREVIEWSTREAM);
    }
}

//Main application
void app_main()
{

    // init spi for the esp32
    init_esp32_spi();

    runDemo();

    //Never reached.
    deinit_esp32_spi();

}
