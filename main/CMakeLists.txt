# Create library
set(SOURCES
    # depthai-shared sources

    # depthai-spi-library sources
    depthai-spi-library/spi_protocol.c
    depthai-spi-library/spi_messaging.c
    # project
    app_main.cpp
    esp32_spi_impl.c
    common/float16.c
    common/decode_mobilenet.c
    SpiPacketParser.cpp
)

set(INCLUDE_DIRS  
    # depthai-shared
    depthai-shared/include
    # depthai-spi-library
    depthai-spi-library
    # project
    common
    .
)

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS ${INCLUDE_DIRS})
