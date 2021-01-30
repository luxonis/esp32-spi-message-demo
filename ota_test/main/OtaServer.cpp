/* HTTP File Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"

#include "FlashMT25Q.hpp"

/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

#define BL_FLASH_START 0
#define DAP_FLASH_START 1024*1024

struct ota_server_data {
    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

static const char *TAG = "OtaServer";

static esp_err_t receive_and_write(httpd_req_t *req, uint32_t writeOffset){
    ESP_LOGI(TAG, "Receiving file...");

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *buf = ((struct ota_server_data *)req->user_ctx)->scratch;
    int received;

    /* Content length of the request gives
     * the size of the file being uploaded */
    int remaining = req->content_len;

    //-----------------------------------------------------------------------------------------------------
    // TODO: Initalize the flash somewhere else and only once...
    FlashMT25Q myExtFlash;
    // erase the region we'll be writing to.
    myExtFlash.eraseRegion(writeOffset, req->content_len);
    ESP_LOGI(TAG, "Region erased! %d to %d", DAP_FLASH_START, DAP_FLASH_START+req->content_len);
    //-----------------------------------------------------------------------------------------------------
    while (remaining > 0) {

        ESP_LOGI(TAG, "Remaining size : %d", remaining);
        /* Receive the file part by part into a buffer */
        if ((received = httpd_req_recv(req, buf, MIN(remaining, SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        //-----------------------------------------------------------------------------------------------------
        myExtFlash.write(buf, writeOffset, received);
        writeOffset += received;
        //-----------------------------------------------------------------------------------------------------

        /* Keep track of remaining size of
         * the file left to be uploaded */
        remaining -= received;
    }

    //-----------------------------------------------------------------------------------------------------
    myExtFlash.readTest(DAP_FLASH_START, 100);
    //-----------------------------------------------------------------------------------------------------

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File uploaded successfully");
    return ESP_OK;
}


/* Handler to upload a ota onto the server */
static esp_err_t upload_bl_post_handler(httpd_req_t *req)
{
    return receive_and_write(req, BL_FLASH_START);
}

/* Handler to upload a ota onto the server */
static esp_err_t upload_dap_post_handler(httpd_req_t *req)
{
    return receive_and_write(req, DAP_FLASH_START);
}


/* Function to start the ota server */
esp_err_t start_ota_server()
{
    static struct ota_server_data *server_data = NULL;

    if (server_data) {
        ESP_LOGE(TAG, "OTA server already started");
        return ESP_ERR_INVALID_STATE;
    }

    /* Allocate memory for server data */
    server_data = (ota_server_data*) calloc(1, sizeof(struct ota_server_data));
    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start OTA server!");
        return ESP_FAIL;
    }


    /* URI handler for uploading files to server */
    httpd_uri_t dap_upload = {
        .uri       = "/upload_dap/*",   // Match all URIs of type /upload/path/to/file
        .method    = HTTP_POST,
        .handler   = upload_dap_post_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &dap_upload);

    /* URI handler for uploading files to server */
    httpd_uri_t bl_upload = {
        .uri       = "/upload_bl/*",   // Match all URIs of type /upload/path/to/file
        .method    = HTTP_POST,
        .handler   = upload_bl_post_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &bl_upload);

    return ESP_OK;
}
