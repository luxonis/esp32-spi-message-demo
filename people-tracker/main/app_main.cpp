#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstring>

#include "decode_raw_mobilenet.h"
#include "esp32_spi_impl.h"

#include "spi_api.hpp"

extern "C" {
   void app_main();
}

const std::string status[4] = {"New", "Tracked", "Lost", "Removed"};

void run_demo(){
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);


    while(1) {
        // ----------------------------------------
        // basic example of receiving tracklets from the MyriadX
        // ----------------------------------------
        dai::Message msg;
        if(mySpiApi.req_message(&msg, "tracklets")){
            // example of parsing the raw metadata
            dai::RawTracklets tracklets;
            mySpiApi.parse_metadata(&msg.raw_meta, tracklets);
            for(const auto& t : tracklets.tracklets){
                printf("ID: %d, Status: %s, xmin: %f, ymin: %f, xmax: %f, ymax: %f\n",
                    t.id, status[(std::int32_t)t.status].c_str(),
                    t.srcImgDetection.xmin, t.srcImgDetection.ymin,
                    t.srcImgDetection.xmax, t.srcImgDetection.ymax);
            }

            // free up resources once you're done with the message.
            mySpiApi.free_message(&msg);
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
