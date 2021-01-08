#ifndef SHARED_SPI_CALLBACKS_H
#define SHARED_SPI_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/igmp.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "soc/rtc_periph.h"
#include "esp32/rom/cache.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_intr_alloc.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include <spi_protocol.h>
#include <spi_messaging.h>


/*
Pins in use. The SPI Master can use the GPIO mux, so feel free to change these if needed.
*/
// DepthAI uses SPI2.6 pin as an interrupt output, open-drain, active low.
#define GPIO_HANDSHAKE 2 // ESP32 pin that connects to SPI2.6

#if 0 // original config in esp-idf example
#define GPIO_MOSI 12
#define GPIO_MISO 13
#define GPIO_SCLK 15
#define GPIO_CS 14C
#else // GPIOs used in the upcoming board: DepthAI with ESP-WROOM-32
#define GPIO_MOSI 13
#define GPIO_MISO 12
#define GPIO_SCLK 14
#define GPIO_CS 15
#endif


void init_esp32_spi();
void deinit_esp32_spi();
uint8_t esp32_send_spi(const char* sendbuf);
uint8_t esp32_recv_spi(char* recvbuf);

#ifdef __cplusplus
}
#endif


#endif
