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
        if(mySpiApi.req_message(&received_msg, METASTREAM)){
            // example of parsing the raw metadata
            dai::RawImgDetections det;
            mySpiApi.parse_metadata(&received_msg.raw_meta, det);
            for(const auto& det : det.detections){
                printf("label: %d, xmin: %f, ymin: %f, xmax: %f, ymax: %f\n", det.label, det.xmin, det.ymin, det.xmax, det.ymax);
            }

            // free up resources once you're done with the message.
            // and pop the message from the queue
            mySpiApi.free_message(&received_msg);
            mySpiApi.spi_pop_messages();
        }
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
