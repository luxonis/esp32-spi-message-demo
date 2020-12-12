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

#include "decode_raw_mobilenet.h"
#include "esp32_spi_impl.h"
#include "spi_api.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define DECODE_RAW_MOBILENET 0
#define DEBUG_METADATA 0

#define MAX_DETECTIONS 16

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
void exampleDecodeRawMobilenet(SpiGetMessageResp *get_message_resp){
    json j;

    Detection dets[MAX_DETECTIONS];

    int num_found = decode_raw_mobilenet(dets, (half *)get_message_resp->data, 0.5f, MAX_DETECTIONS);
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
// example callback for chunking up large messages.
// ----------------------------------------
uint32_t example_chunk_recv_size = 0;
void example_chunk_message(char* received_packet, uint32_t packet_size, uint32_t message_size){
    example_chunk_recv_size += packet_size;

    printf("example_chunk_message called back packet %d/%d\n", example_chunk_recv_size, message_size);

    if(example_chunk_recv_size >= message_size){
        example_chunk_recv_size = 0;
    }
}
// ----------------------------------------


void run_demo(){
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
        // basic example of receiving data and metadata from messages.
        // ----------------------------------------
        FullMessage received_msg;
        uint8_t recv_success = mySpiApi.req_full_msg(&received_msg, METASTREAM);
        if(recv_success){
            // example of decoding mobilenet (only for use with raw mobilenet tensor output).
            if(DECODE_RAW_MOBILENET){
                exampleDecodeRawMobilenet(&received_msg.raw_data_resp);
            }

            // print out plain text metadata (for debug purposes).
            if(DEBUG_METADATA){
                json j = json::from_msgpack(received_msg.raw_meta_resp.data, received_msg.raw_meta_resp.data + received_msg.raw_meta_resp.data_size);
                printf("Unpacked metadata: %s\n", j.dump().c_str());
            }

            // free up resources once you're done with the message.
            mySpiApi.free_full_msg(&received_msg);
        }


        // ----------------------------------------
        // example of getting large messages a chunk/packet at a time.
        // ----------------------------------------
        //mySpiApi.set_chunk_packet_cb(&example_chunk_message);
        //mySpiApi.chunk_message(METASTREAM);

        // ----------------------------------------
        // pop current message/metadata. this tells the depthai to update the info being passed back using the spi_cmds.
        // ----------------------------------------
        req_success = mySpiApi.spi_pop_messages();
        // or you can pop them individually, like this:
        //req_success = mySpiApi.spi_pop_message(METASTREAM);
        //req_success = mySpiApi.spi_pop_message(PREVIEWSTREAM);
    }
}

//Main application
void app_main()
{

    // init spi for the esp32
    init_esp32_spi();

    run_demo();

    //Never reached.
    deinit_esp32_spi();

}
