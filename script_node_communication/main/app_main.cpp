#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp32_spi_impl.h"

#include "spi_api.hpp"
#include <cmath>

extern "C"
{
    void app_main();
}

void run_demo()
{
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);

    sleep(5);

    while (1)
    {
        dai::Message nnMsg;
        if(mySpiApi.req_message(&nnMsg, "spimetaout")){
            dai::RawNNData nnData;
            mySpiApi.parse_metadata(&nnMsg.raw_meta, nnData);
            printf("Tensors size: %u\n",nnData.tensors.size());
            for(int i=0; i<nnData.tensors.size(); i++) {
            // for(const auto& tensor : nnData.tensors){
                auto tensor = nnData.tensors[i];
                printf("============= i=%u ===========", i);
                printf("Dimensions %u\n", tensor.numDimensions);
                size_t size = tensor.dims[0] * tensor.strides[0];
                printf("name of the sensor :%s\n",tensor.name.c_str());
                printf("size of the tensor: %u\n", size);
                auto beg = nnData.data.begin() + tensor.offset;
                auto end = beg + size;
                size_t data_size = nnData.data.size();
                printf("size of the data :%d, ptr: %08X\n", nnMsg.raw_data.size, (uint32_t) nnMsg.raw_data.data);

                printf("msg: %d\n", nnMsg.raw_data.data[0]);

            }
            // free up resources once you're done with the message.
            mySpiApi.free_message(&nnMsg);
            mySpiApi.spi_pop_messages();
        } else {
            // Periodically check if new messages are available
            usleep(100);
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