#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_system.h"

class FlashMT25Q {
    private:
        esp_flash_t* ext_flash;
    public:
        FlashMT25Q();
        ~FlashMT25Q();

        // debug stuff
        void read(char* readBuf, uint32_t readAddr, uint32_t readLen);
        void write(char* writeBuf, uint32_t writeAddr, uint32_t writeLen);
        void eraseRegion(uint32_t eraseAddr, uint32_t eraseLen);
        void readTest(uint32_t readAddr, uint32_t readLen);
        void writeTest(uint32_t writeAddr, uint32_t writeLen);
};

