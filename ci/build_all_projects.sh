#!/bin/bash

# Exit if any subsequent commands fail
set -e

# Source folder of this script
SCRIPT_PATH=$(realpath $0)
echo $SCRIPT_PATH
SOURCE_DIR=$(dirname $SCRIPT_PATH)
echo $SOURCE_DIR
ROOT_DIR="$SOURCE_DIR/../"
echo $ROOT_DIR

### Projects

PROJECTS=(
    gpio_interrupts image_part jpeg_demo jpeg_webserver_demo mjpeg_streaming_wifi
    mobilenet_raw parse_meta people_tracker script_node_communication spatial_image_detections
    spatial_location_calculator speed_benchmark spi_in_landmark spi_in_passthrough two_streams
)

for project in "${PROJECTS[@]}"; do
    cd "$ROOT_DIR/$project"
    idf.py build
done
