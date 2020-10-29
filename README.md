ESP32 app build & flash with esp-idf:
idf.py build
idf.py -p /dev/ttyUSB1 flash

Open a serial terminal to see the prints, for example:
cu -l /dev/ttyUSB0 -s 115200
