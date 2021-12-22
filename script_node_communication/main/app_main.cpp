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
    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);

    sleep(5);

    // Create RawBuffer message and set some text to it
    dai::RawBuffer buf;
    std::string str = "Hello";
    std::vector<uint8_t> strVec(str.begin(), str.end());
    buf.data = strVec;

    // Send message and wait for the return message
    dai::Message msg;
    while(!mySpiApi.req_message(&msg, "ret_str")) {
        std::cout << "Sending string '" << str << "' to VPU..." << std::endl;
        mySpiApi.send_message(buf, "str");
        sleep(1);
    }

    std::string retStr(msg.raw_data.data, msg.raw_data.data + msg.raw_data.size);
    std::cout << "String recieved from VPU: '" << retStr << "'" << std::endl;

    // free up resources once you're done with the message.
    mySpiApi.free_message(&msg);
    mySpiApi.spi_pop_messages();
}

//Main application
void app_main()
{
    init_esp32_spi();
    run_demo();
    deinit_esp32_spi();
}