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

# image_part
cd $ROOT_DIR/image_part
idf.py build

# jpeg_demo
cd $ROOT_DIR/jpeg_demo
idf.py build

# jpeg_webserver_demo
cd $ROOT_DIR/jpeg_webserver_demo
idf.py build

# mobilenet_raw
cd $ROOT_DIR/mobilenet_raw
idf.py build

# parse_meta
cd $ROOT_DIR/parse_meta
idf.py build

# spi_in_landmark
cd $ROOT_DIR/spi_in_landmark
idf.py build

# spi_in_passthrough
cd $ROOT_DIR/spi_in_passthrough
idf.py build

# gpio_interrupts
cd $ROOT_DIR/gpio_interrupts
idf.py build

# speed_benchmark
cd $ROOT_DIR/speed_benchmark
idf.py build