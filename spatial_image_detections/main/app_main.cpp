#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "decode_raw_mobilenet.h"
#include "esp32_spi_impl.h"

#include "spi_api.hpp"

static const char* METASTREAM = "spimetaout";

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
        req_success = mySpiApi.req_message(&received_msg, METASTREAM);
        if(req_success){
            // example of parsing the raw metadata
            dai::RawSpatialImgDetections det;
            mySpiApi.parse_metadata(&received_msg.raw_meta, det);
            for(const auto& det : det.detections){
                printf("label: %d, xmin: %.2f, ymin: %.2f, xmax: %.2f, ymax: %.2f, x: %.2f, y: %.2f, z: %.2f\n", det.label, det.xmin, det.ymin, det.xmax, det.ymax, det.spatialCoordinates.x, det.spatialCoordinates.y, det.spatialCoordinates.z);
            }

            // free up resources once you're done with the message.
            mySpiApi.free_message(&received_msg);
        }

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
