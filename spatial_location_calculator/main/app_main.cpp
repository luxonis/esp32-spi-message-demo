#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp32_spi_impl.h"

#include "spi_api.hpp"
#include <cmath>

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
            dai::RawSpatialLocations rawSpatialLocations;
            mySpiApi.parse_metadata(&received_msg.raw_meta, rawSpatialLocations);
            for(const auto& spatialData : rawSpatialLocations.spatialLocations){
                auto avgDepth = spatialData.depthAverage;
                auto depthAveragePixelCount = spatialData.depthAveragePixelCount;
                auto x = spatialData.spatialCoordinates.x;
                auto y = spatialData.spatialCoordinates.y;
                auto z = spatialData.spatialCoordinates.z;
                auto euclideanDistance = std::sqrt(x*x + y*y + z*z);
                printf("Euclidean distance %d mm, X: %d mm, Y: %d mm, Z: %d mm \n",(int)euclideanDistance,(int)x,(int)y,(int)z);
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
