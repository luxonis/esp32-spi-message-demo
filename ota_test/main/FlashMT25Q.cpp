#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <FlashMT25Q.hpp>

#include <esp_log.h>
#include <esp32_spi_impl.h>

#include <driver/gpio.h>

static const char *TAG = "FlashMT25Q";
#define PIPELINE_OFFSET 1024*1024

FlashMT25Q::~FlashMT25Q(){
}

FlashMT25Q::FlashMT25Q(){
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
        .io_mode = SPI_FLASH_DIO,
        .speed = ESP_FLASH_40MHZ,
        .input_delay_ns = 0,
        .cs_id = 0,
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
    ESP_ERROR_CHECK(spi_bus_add_flash_device(&ext_flash, &device_config));

    // Probe the Flash chip and initialize it
    esp_err_t err = esp_flash_init(ext_flash);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external Flash: %s (0x%x)", esp_err_to_name(err), err);
        return;
    }

    // Print out the ID and size
    uint32_t id;
    ESP_ERROR_CHECK(esp_flash_read_id(ext_flash, &id));

    // esp_flash_init doesn't calculate chip size for MT25QU01GBBB correctly so we'll manually set it here.
    if(id==0x20bb21){
        ext_flash->size = 1024*1024*1024;
    }
    ESP_LOGI(TAG, "Initialized external Flash, size=%d KB, ID=0x%x", ext_flash->size / 1024, id);
}

void FlashMT25Q::read(char* readBuf, uint32_t readAddr, uint32_t readLen){
    ESP_ERROR_CHECK(esp_flash_read(ext_flash, readBuf, readAddr, readLen));
}

void FlashMT25Q::write(char* writeBuf, uint32_t writeAddr, uint32_t writeLen){
    ESP_ERROR_CHECK(esp_flash_write(ext_flash, writeBuf, writeAddr, writeLen));
}

void FlashMT25Q::eraseRegion(uint32_t eraseAddr, uint32_t eraseLen){
    // disable write protect.
    esp_flash_set_chip_write_protect(ext_flash, false);

    // erase first... adjust erase length based off writeLen. for MT25QU01GBBB flash we have 4k sectors ((writeLen-1)>>12).
    uint32_t eraseLenPadded = 4096 * ((eraseLen-1)>>12) + 4096;
    ESP_ERROR_CHECK(esp_flash_erase_region(ext_flash, eraseAddr, eraseLenPadded));
}


void FlashMT25Q::readTest(uint32_t readAddr, uint32_t readLen){
    // read test...
    char *readBuf = (char *) malloc(readLen);
    ESP_ERROR_CHECK(esp_flash_read(ext_flash, readBuf, readAddr, readLen));

    int entriesPerLine = 8;
    for(int i=0; i<readLen; i++){
        printf("%02hhX\t", readBuf[i]);
        if((i+1)%entriesPerLine==0){
            printf("\n");
        }
    }
    printf("\n");

    free(readBuf);
}

void FlashMT25Q::writeTest(uint32_t writeAddr, uint32_t writeLen){
    // disable write protect.
    esp_flash_set_chip_write_protect(ext_flash, false);

    // erase first... adjust erase length based off writeLen. for MT25QU01GBBB flash we have 4k sectors ((writeLen-1)>>12).
    uint32_t eraseLen = 4096 * ((writeLen-1)>>12) + 4096;
    ESP_ERROR_CHECK(esp_flash_erase_region(ext_flash, writeAddr, eraseLen));

    // write test...
    char *writeBuf = (char *) malloc(writeLen);
    for(int i=0; i<writeLen; i++){
        writeBuf[i] = i%256;
    }
    printf("\n");

    ESP_ERROR_CHECK(esp_flash_write(ext_flash, writeBuf, writeAddr, writeLen));

    free(writeBuf);
}


