# Create library
set(SOURCES
    main.cpp
    OtaServer.cpp
    FlashMT25Q.cpp
)

set(INCLUDE_DIRS  
    .
    $ENV{IDF_PATH}/examples/common_components/protocol_examples_common/include
)


set(REQUIRES  
    esp_http_server
    depthai-spi-api
    protocol_examples_common
    spiffs
)

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS ${INCLUDE_DIRS} REQUIRES ${REQUIRES} PRIV_REQUIRES ${PRIV_REQUIRES} EMBED_FILES "favicon.ico" "upload_script.html")
