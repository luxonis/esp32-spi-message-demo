# Simple MJPEG Streaming Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

## Usage

* Open the project configuration menu (`idf.py menuconfig`) go to `Example Configuration` ->
    1. WIFI SSID: WIFI network to which your PC is also connected to.
    2. WIFI Password: WIFI password

* In order to test the file server demo :
    1. compile and burn the firmware `idf.py -p PORT flash`
    2. run `idf.py -p PORT monitor` and note down the IP assigned to your ESP module. The default port is 80
    3. run `python3 main.py` or `python3 main_lightweight.py` to start DepthAI program
    3. test the example interactively on a web browser (assuming IP is 192.168.43.130):
        1. open path `http://192.168.43.130/getframe/`
        2. see the MJPEG stream playing
