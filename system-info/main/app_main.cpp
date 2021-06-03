#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstring>

#include "esp32_spi_impl.h"
#include "spi_api.hpp"

extern "C" {
   void app_main();
}

void run_demo(){
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);

    auto m = 1024.0 * 1024.0;
    while(1) {
        // ----------------------------------------
        // basic example of receiving system information from the MyriadX
        // ----------------------------------------
        dai::Message msg;
        if(mySpiApi.req_message(&msg, "sysinfo")){
            // example of parsing the raw metadata
            dai::RawSystemInformation info;
            mySpiApi.parse_metadata(&msg.raw_meta, info);

            printf("DDR used / total - %f / %f MiB\n", (info.ddrMemoryUsage.used / m), (info.ddrMemoryUsage.total / m));
            printf("Cmx used / total - %f / %f MiB\n", (info.cmxMemoryUsage.used / m), (info.cmxMemoryUsage.total / m));
            printf("LeonCss heap used / total - %f / %f MiB\n", (info.leonCssMemoryUsage.used / m), (info.leonCssMemoryUsage.total / m));
            printf("LeonMss heap used / total - %f / %f MiB\n", (info.leonMssMemoryUsage.used / m), (info.leonMssMemoryUsage.total / m));
            auto t = info.chipTemperature;
            printf("Chip temperature - average: %f, css: %f, mss: %f, upa0: %f, upa1: %f\n", t.average, t.css, t.mss, t.upa, t.dss);
            printf("Cpu usage - Leon OS: %f Leon RT: %f\n\n", (info.leonCssCpuUsage.average * 100), (info.leonMssCpuUsage.average * 100));
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
