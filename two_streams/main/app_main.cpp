#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp32_spi_impl.h"

#include "spi_api.hpp"
#include <cmath>

extern "C" {
   void app_main();
}

void run_demo(){
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);

    bool receivedAnyMessage = false;
    while(1) {
        dai::Message spatialDataMsg;

        if(mySpiApi.req_message(&spatialDataMsg, "spatialData")){
            // example of parsing the raw metadata
            dai::RawSpatialLocations rawSpatialLocations;
            mySpiApi.parse_metadata(&spatialDataMsg.raw_meta, rawSpatialLocations);
            for(const auto& spatialData : rawSpatialLocations.spatialLocations){
                // auto avgDepth = spatialData.depthAverage;
                // auto depthAveragePixelCount = spatialData.depthAveragePixelCount;
                auto x = spatialData.spatialCoordinates.x;
                auto y = spatialData.spatialCoordinates.y;
                auto z = spatialData.spatialCoordinates.z;
                auto euclideanDistance = std::sqrt(x*x + y*y + z*z);
                printf("Euclidean distance %d mm, X: %d mm, Y: %d mm, Z: %d mm \n",(int)euclideanDistance,(int)x,(int)y,(int)z);
            }
            // free up resources once you're done with the message.
            mySpiApi.free_message(&spatialDataMsg);
            mySpiApi.spi_pop_message("spatialData");
            receivedAnyMessage = true;
        }

        dai::Message nnMsg;
        if(mySpiApi.req_message(&nnMsg, "nn")){
            dai::RawImgDetections nnDetections;
            mySpiApi.parse_metadata(&nnMsg.raw_meta, nnDetections);
            for(const auto& det : nnDetections.detections){
                printf("label: %d, xmin: %.2f, ymin: %.2f, xmax: %.2f, ymax: %.2f\n", det.label, det.xmin, det.ymin, det.xmax, det.ymax);
            }
            // free up resources once you're done with the message.
            mySpiApi.free_message(&nnMsg);
            mySpiApi.spi_pop_message("nn");
            receivedAnyMessage = true;
        }

        if(!receivedAnyMessage){
            // Delay pooling of messages
            usleep(1000);
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
