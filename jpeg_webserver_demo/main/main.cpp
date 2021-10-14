/* HTTP File Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sys/param.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"





//-------------------------------------------------------------------------------------------------------------------
#include <main.hpp>

#include "esp32_spi_impl.h"
#include "spi_api.hpp"

#define MAX_DETECTIONS 16

static const char* METASTREAM = "spimetaout";
static const char* PREVIEWSTREAM = "spipreview";

const char *base_path = "/spiffs";
const char *jpeg_path = "/spiffs/frame.jpg";

extern "C" {
   void app_main();
}

// example callback for chunking up large messages.
uint32_t example_chunk_recv_size = 0;
FILE* JPEG_FILE = NULL;
bool write_jpeg = false;

uint8_t skip_print = 0;
void example_chunk_message(void* received_packet, uint32_t packet_size, uint32_t message_size){

    // if write file var is set and it's the first packet of a message...
    if( write_jpeg && !JPEG_FILE && example_chunk_recv_size == 0){
        // open up the jpeg file for writing.
        printf("starting new jpeg write...\n");
        JPEG_FILE = fopen(jpeg_path, "w");

        if (!JPEG_FILE) {
            printf("Failed to create file : %s", jpeg_path);
        }
    }

    example_chunk_recv_size += packet_size;
    if(skip_print%16 == 0){
        printf("example_chunk_message called back packet %d/%d\n", example_chunk_recv_size, message_size);
        skip_print = 0;
    }
    skip_print++;

    // open up the jpeg file for writing.
    if(write_jpeg && JPEG_FILE){
        fwrite(received_packet, sizeof(char), packet_size, JPEG_FILE);
    }

    // if it's the last packet of a message...
    if(example_chunk_recv_size >= message_size){
        example_chunk_recv_size = 0;
        if (write_jpeg && JPEG_FILE){
            // close up the jpeg file.
            fclose(JPEG_FILE);
            JPEG_FILE = NULL;
            write_jpeg = false;
        }
    }
}

void start_depthai_spi(){
    uint8_t req_success = 0;

    dai::SpiApi mySpiApi;
    mySpiApi.set_send_spi_impl(&esp32_send_spi);
    mySpiApi.set_recv_spi_impl(&esp32_recv_spi);

    while(1) {
        // ----------------------------------------
        // example of getting large messages a chunk/packet at a time.
        // ----------------------------------------
        mySpiApi.set_chunk_packet_cb(&example_chunk_message);
        mySpiApi.chunk_message(PREVIEWSTREAM);

        // ----------------------------------------
        // pop current message/metadata. this tells the depthai to update the info being passed back using the spi_cmds.
        // ----------------------------------------
        req_success = mySpiApi.spi_pop_messages();
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
