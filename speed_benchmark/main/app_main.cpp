
// std
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <chrono>

// libraries
#include "decode_raw_mobilenet.h"
#include "esp32_spi_impl.h"
#include "spi_api.hpp"

static const char* PREVIEWSTREAM = "spipreview";

// example callback for chunking up large messages.
auto t1 = std::chrono::steady_clock::now();
uint32_t example_chunk_recv_size = 0;
void example_chunk_message(void* received_packet, uint32_t packet_size, uint32_t message_size){
    example_chunk_recv_size += packet_size;

    if(std::chrono::steady_clock::now() - t1 > std::chrono::seconds(1)){
        t1 = std::chrono::steady_clock::now();
        printf("transfered %dkiB/s\n", example_chunk_recv_size / 1024);
        example_chunk_recv_size = 0;
    }

}


void run_demo(){
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);

    while(true) {
        // ----------------------------------------
        // example of getting large messages a chunk/packet at a time.
        // ----------------------------------------
        mySpiApi.set_chunk_packet_cb(&example_chunk_message);
        if(mySpiApi.chunk_message(PREVIEWSTREAM)){
            // Successfully received the message, remove it from the queue

            // ----------------------------------------
            // pop current message/metadata. this tells the depthai to update the info being passed back using the spi_cmds.
            // ----------------------------------------
            req_success = mySpiApi.spi_pop_message(PREVIEWSTREAM);
        }
    }
}

//Main application
extern "C" void app_main()
{

    // init spi for the esp32
    init_esp32_spi();

    run_demo();

    //Never reached.
    deinit_esp32_spi();

}
