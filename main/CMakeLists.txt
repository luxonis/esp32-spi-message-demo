# Create library
set(SOURCES
    app_main.cpp
    esp32_spi_impl.c
)

set(INCLUDE_DIRS  
    .
)

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS ${INCLUDE_DIRS} REQUIRES depthai-spi-api)
