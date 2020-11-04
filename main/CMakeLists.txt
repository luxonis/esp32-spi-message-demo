idf_component_register(SRCS "app_main.c" "depthai-spi-library/spi_protocol.c" "depthai-spi-library/spi_messaging.c" "esp32_spi_impl.c" "common/float16.c" "common/decode_mobilenet.c"
                    INCLUDE_DIRS "." "depthai-spi-library" "common" )
