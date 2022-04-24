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
// example of decoding raw mobilenet output
// Also see included decode_raw_mobilenet.h.
// ----------------------------------------
void exampleDecodeRawMobilenet(uint8_t *data){
    Detection dets[MAX_DETECTIONS];

    int num_found = decode_raw_mobilenet(dets, (half *)data, 0.5f, MAX_DETECTIONS);
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

void run_demo(){
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);


    while(1) {
        // ----------------------------------------
        // basic example of receiving data and metadata from messages.
        // ----------------------------------------
        dai::Message received_msg;
        req_success = mySpiApi.req_message(&received_msg, METASTREAM);
        if(req_success){
            // example of decoding mobilenet (only for use with raw mobilenet tensor output).
            exampleDecodeRawMobilenet(received_msg.raw_data.data);

            // free up resources once you're done with the message.
            mySpiApi.free_message(&received_msg);
        }

        // ----------------------------------------
        // pop current message/metadata. this tells the depthai to update the info being passed back using the spi_cmds.
        // ----------------------------------------
        req_success = mySpiApi.spi_pop_messages();

        usleep(30000);
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
