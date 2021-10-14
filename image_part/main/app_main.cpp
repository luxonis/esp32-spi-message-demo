#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp32_spi_impl.h"

#include "spi_api.hpp"

static const char* PREVIEWSTREAM = "spipreview";

extern "C" {
   void app_main();
}

void run_demo(){
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);

    int newlineiter = 0;
    int newlinemod = 20;
    int offset = 269900;
    int dataSize = 100;
    while(1) {
        dai::Data received_data;
        req_success = mySpiApi.req_data_partial(&received_data, PREVIEWSTREAM, offset, dataSize);
        if(req_success){
            newlineiter = 0;
            for(int i=0; i<dataSize; i++){
                printf("%d\t", received_data.data[i]);

                newlineiter++;
                if(newlineiter%newlinemod == 0){
                    printf("\n");
                }
            }
            printf("\n");
            // free up resources once you're done with the message.
            free(received_data.data);
            // pop message after reading
            mySpiApi.spi_pop_message(PREVIEWSTREAM);
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
