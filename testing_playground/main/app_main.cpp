
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

/*
static const char* PREVIEWSTREAM = "spipreview";

// example callback for chunking up large messages.
auto t1 = std::chrono::steady_clock::now();
uint32_t example_chunk_recv_size = 0;
void example_chunk_message(char* received_packet, uint32_t packet_size, uint32_t message_size){
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

*/

//std::array<uint8_t, 256 * 1024> large_send_buffer;
std::array<uint8_t, 100 * 1024> large_receive_buffer __attribute__((__aligned__(64)));

std::array<uint8_t, 128> small_receive_buffer __attribute__((__aligned__(64)));


//Main application
extern "C" void app_main()
{
    using namespace std::chrono;

    // init spi for the esp32
    init_esp32_spi();


    // prepare buffers
    // for(unsigned i = 0; i < large_send_buffer.size(); i++){
    //     //large_send_buffer[i] = i+1;
    //     large_receive_buffer[i] = 0;
    // }

    auto t = steady_clock::now();

    // for(int i = 0; i < 32; i++){
    //     esp32_transfer_spi(large_send_buffer.data() + 1024 * i * 2, 1024 * 2, large_receive_buffer.data() + 1024*i*2, 1024*2);
    //     //usleep(1);
    // }

    //esp32_enable_spi_cs(1);
    bool success = true;
    esp32_transfer_spi(nullptr, 0 /*large_send_buffer.data(), large_send_buffer.size()*/, large_receive_buffer.data(), large_receive_buffer.size());
    for(unsigned i = 0; i < large_receive_buffer.size(); i++){
        if(i % 2 == 0){
            if(large_receive_buffer[i] != 0b10101010) success = false;
        } else {
            if(large_receive_buffer[i] != 0b01010101) success = false;
        }
        if(!success) {
            printf("Receive buffer not correct at byte index = %d...\n", i);
            success = false;
            break;
        }
    }

    usleep(5000);

    esp32_transfer_spi(nullptr, 0 /*large_send_buffer.data(), large_send_buffer.size()*/, large_receive_buffer.data(), large_receive_buffer.size());
    for(unsigned i = 0; i < large_receive_buffer.size(); i++){
        if(i % 2 == 0){
            if(large_receive_buffer[i] != 0b10101010) success = false;
        } else {
            if(large_receive_buffer[i] != 0b01010101) success = false;
        }
        if(!success) {
            printf("Receive buffer not correct at byte index = %d...\n", i);
            success = false;
            {
                int offset = 0;
                for(unsigned i = offset; i < offset + 550; i++){
                 printf("%02X ", large_receive_buffer[i]);
                }
                printf("\n");
            }
            break;
        }
    }
    //esp32_enable_spi_cs(0);

    usleep(5000);

    esp32_transfer_spi(nullptr, 0 /*large_send_buffer.data(), large_send_buffer.size()*/, small_receive_buffer.data(), small_receive_buffer.size());

    auto bytesps = (large_receive_buffer.size() * 2 + small_receive_buffer.size()) * seconds(1) / (steady_clock::now() - t);
    printf("estimated kiB/s: %lld\n", bytesps / 1024);
    // estimated speed


    // {
    //     int offset = 0;
    //     for(unsigned i = offset; i < offset + 550; i++){
    //         printf("%02X ", large_receive_buffer[i]);
    //     }
    //     printf("\n");
    // }



    // Check received data
    for(unsigned i = 0; i < small_receive_buffer.size(); i++){
        if((uint8_t)(i) != small_receive_buffer[i]){
            printf("Small receive buffer not correct at byte index = %d...\n", i);
            success = false;
            break;
        }
    }

    {
        int offset = 0;
        for(unsigned i = offset; i < offset + 128; i++){
            printf("%02X ", small_receive_buffer[i]);
        }
        printf("\n");
    }


    //run_demo();

    //Never reached.
    deinit_esp32_spi();

}
