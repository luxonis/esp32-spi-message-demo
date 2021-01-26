#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <esp32_spi_impl.h>

#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_system.h"

static const char *TAG = "example";

extern "C" {
   void app_main();
}


static esp_flash_t* example_init_ext_flash(void)
{
    const spi_bus_config_t bus_config = {
        .mosi_io_num = VSPI_IOMUX_PIN_NUM_MOSI,
        .miso_io_num = VSPI_IOMUX_PIN_NUM_MISO,
        .sclk_io_num = VSPI_IOMUX_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    const esp_flash_spi_device_config_t device_config = {
        .host_id = VSPI_HOST,
        .cs_io_num = VSPI_IOMUX_PIN_NUM_CS,
        .io_mode = SPI_FLASH_QIO,
        .speed = ESP_FLASH_40MHZ,
        .input_delay_ns = 0,
        .cs_id = 0
    };

    ESP_LOGI(TAG, "Initializing external SPI Flash");
    ESP_LOGI(TAG, "Pin assignments:");
    ESP_LOGI(TAG, "MOSI: %2d   MISO: %2d   SCLK: %2d   CS: %2d",
        bus_config.mosi_io_num, bus_config.miso_io_num,
        bus_config.sclk_io_num, device_config.cs_io_num
    );

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &bus_config, 1));

    // Add device to the SPI bus
    esp_flash_t* ext_flash;
    ESP_ERROR_CHECK(spi_bus_add_flash_device(&ext_flash, &device_config));

    // Probe the Flash chip and initialize it
    esp_err_t err = esp_flash_init(ext_flash);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external Flash: %s (0x%x)", esp_err_to_name(err), err);
        return NULL;
    }

    // Print out the ID and size
    uint32_t id;
    ESP_ERROR_CHECK(esp_flash_read_id(ext_flash, &id));
    ESP_LOGI(TAG, "Initialized external Flash, size=%d B, ID=0x%x", ext_flash->size, id);

    return ext_flash;
}


//Main application
void app_main()
{
    esp_flash_t* flash = example_init_ext_flash();

//    if(esp_flash_chip_driver_initialized(flash)){
//        printf("asdfasdf initialized...\n");
//    }

//    uint32_t flashSize;
//    esp_flash_get_size(flash, &flashSize);
//    printf("asdfasdf flashSize = %d\n", flashSize);


}
