#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "decode_raw_mobilenet.h"
#include "esp32_spi_impl.h"

#include "spi_api.hpp"

#define MAX_DETECTIONS 16

static const char* METASTREAM = "spimetaout";
static const char* PREVIEWSTREAM = "spipreview";

extern "C" {
   void app_main();
}


// ----------------------------------------
// example callback for chunking up large messages.
// ----------------------------------------
uint32_t example_chunk_recv_size = 0;
uint32_t skip_print_cnt = 0;
void example_chunk_message(char* received_packet, uint32_t packet_size, uint32_t message_size){
    example_chunk_recv_size += packet_size;

    // print contents...
//    for(int i=0; i<packet_size; i++){
//        if(i%40==0){
//            printf("\n");
//        }
//        printf("%02x", received_packet[i]);
//    }
//    printf("\n");


    if(skip_print_cnt%8 == 0){
        printf("example_chunk_message called back packet %d/%d\n", example_chunk_recv_size, message_size);
    }
    skip_print_cnt++;

    if(example_chunk_recv_size >= message_size){
        example_chunk_recv_size = 0;
    }
}
// ----------------------------------------

void run_demo(){
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);


    // initialize some test data (first byte of each packet will increment...)
    std::vector<std::uint8_t> contents(97200, 0);
    uint8_t packCount = 0;
    for(int i=0; i<97200; i++){
        if(i%252==0){
            contents[i] = packCount;
            packCount++;
        }
    }

    // create a Raw/Meta
    dai::RawImgFrame img_frame;
    img_frame.fb.width = 180;
    img_frame.fb.height = 180;
    img_frame.fb.stride = 180;
    img_frame.fb.bytesPP = 1;
    img_frame.fb.p1Offset = 32400;
    img_frame.fb.p2Offset = 64800;
    img_frame.fb.p3Offset = 97200;

    img_frame.ts.sec = 1;
    img_frame.ts.nsec = 123456789;

    img_frame.category = 0;
    img_frame.instanceNum = 1;
    img_frame.sequenceNum = 123;

    // copy the frame data
    img_frame.data = contents;

    // just sending the same data over and over again for now.
    while(1) {
        printf("Sending input\n");
        mySpiApi.send_message(img_frame, "spiin");

        // ----------------------------------------
        // example of getting large messages a chunk/packet at a time.
        // ----------------------------------------
        printf("Receiving response\n");
        mySpiApi.set_chunk_packet_cb(&example_chunk_message);
        mySpiApi.chunk_message("spimeta");

        // ----------------------------------------
        // pop current message/metadata. this tells the depthai to update the info being passed back using the spi_cmds.
        // ----------------------------------------
        req_success = mySpiApi.spi_pop_messages();
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
