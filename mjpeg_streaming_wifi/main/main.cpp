/* HTTP MJPEG streaming example */

#include <sys/param.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include <mutex>
#include <condition_variable>
//-------------------------------------------------------------------------------------------------------------------
#include <main.hpp>

#include "esp32_spi_impl.h"
#include "spi_api.hpp"

static const char* PREVIEWSTREAM = "spipreview";

const char *base_path = "/spiffs";

extern "C" {
   void app_main();
}

// example callback for chunking up large messages.
uint32_t example_chunk_recv_size = 0;
std::mutex receiverSocketsMtx;
std::vector<int> receiverSockets;
std::condition_variable receiverSocketsCv;

// Handle /getframe GET requests - MJPEG streaming
esp_err_t get_frame_handler(httpd_req_t *req) {

    // Send an initial header
    const char INITIAL[] = \
        "HTTP/1.1 200 OK\r\n" \
        "Content-type: multipart/x-mixed-replace; boundary=--jpgboundary\r\n" \
        "\r\n"
    ;
    httpd_send(req, INITIAL, strlen(INITIAL));

    // Add socket to the list of receivers
    int socket = httpd_req_to_sockfd(req);

    {
        std::unique_lock<std::mutex> lock(receiverSocketsMtx);
        receiverSockets.push_back(socket);
        receiverSocketsCv.notify_all();
    }

    return ESP_OK;
}


void mjpeg_streaming_message(void* received_packet, uint32_t packet_size, uint32_t message_size){

    std::unique_lock<std::mutex> lock(receiverSocketsMtx);
    for(auto it = receiverSockets.begin(); it != receiverSockets.end(); ){
        auto& socket = *it;
        bool socketError = false;

        // if start of message
        if(example_chunk_recv_size == 0){
            printf("Sending out new mjpeg image..., message size: %d\n", message_size);

            const char ITERATION[] = \
                "--jpgboundary\r\n" \
                "Content-type: image/jpeg\r\n"
            ;
            char contentLength[256];
            memset(contentLength, 0, sizeof(contentLength));
            snprintf(contentLength, sizeof(contentLength), "Content-length: %d\r\n\r\n", message_size);

            // Send without null terminator
            // httpd_send(request.load(), ITERATION, sizeof(ITERATION) - 1);
            if(write(socket, ITERATION, sizeof(ITERATION) - 1) <= 0){
                socketError = true;
            }
            // Send content length
            //httpd_send(request.load(), contentLength, strlen(contentLength));
            if(write(socket, contentLength, strlen(contentLength)) <= 0){
                socketError = true;
            }
        }

        //httpd_send(request.load(), (const char*) received_packet, packet_size);
        if(write(socket, (const char*) received_packet, packet_size) <= 0){
            socketError = true;
        }
        if(example_chunk_recv_size + packet_size >= message_size){
            // last transfer? Add a blank line
            const char END_HEADERS[] = "\r\n";
            //httpd_send(request.load(), END_HEADERS, strlen(END_HEADERS));
            if(write(socket, END_HEADERS, strlen(END_HEADERS)) <= 0){
                socketError = true;
            }
        }

        if(socketError){
            // remove this socket from the vector
            it = receiverSockets.erase(it);
        } else {
            // continue through vector
            it++;
        }

    }

    // if it's the last packet of a message...
    example_chunk_recv_size += packet_size;
    if(example_chunk_recv_size >= message_size){
        example_chunk_recv_size = 0;
    }
}

void start_depthai_spi(){

    // Creates a streaming job. If any receivers are there, start reading and sending the images
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);
    mySpiApi.set_chunk_packet_cb(&mjpeg_streaming_message);

    std::vector<uint8_t> temporaryBuffer;
    temporaryBuffer.resize(64 * 1024);

    int counter = 0;
    while(1) {

        // Wait for CV to notify us about an added stream receiver
        {
            std::unique_lock<std::mutex> lock(receiverSocketsMtx);
            receiverSocketsCv.wait(lock, []{ return receiverSockets.size() > 0; });
        }

        if(mySpiApi.chunk_message_buffer(PREVIEWSTREAM, temporaryBuffer.data(), temporaryBuffer.size())) {
            printf("Correctly sent out mjpeg\n");

            // ----------------------------------------
            // pop current message/metadata. this tells the depthai to update the info being passed back using the spi_cmds.
            // ----------------------------------------
            req_success = mySpiApi.spi_pop_messages();//(PREVIEWSTREAM);

        } else {
            // wait a little
            usleep(100);
        }

    }
}

extern "C" {
   void app_main();
}
//-------------------------------------------------------------------------------------------------------------------


/* This example demonstrates how to create file server
 * using esp_http_server. This file has only startup code.
 * Look in file_server.c for the implementation */

static const char *TAG="example";

/* Function to initialize SPIFFS */
static esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,   // This decides the maximum number of files that can be created on the storage
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

/* Declare the function which starts the file server.
 * Implementation of this function is to be found in
 * file_server.c */
esp_err_t start_file_server(const char *base_path);

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* Initialize file storage */
    ESP_ERROR_CHECK(init_spiffs());

    /* Start the file server */
    ESP_ERROR_CHECK(start_file_server(base_path));



//----------------------------------------------------------------------------------------------
    // init spi for the esp32
    init_esp32_spi();

    start_depthai_spi();

    //Never reached.
    deinit_esp32_spi();
//----------------------------------------------------------------------------------------------


}
