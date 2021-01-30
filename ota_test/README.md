# OTA Test
This is an OTA test that will eventually become an OTA client example. It currently just listens for a POST with attached binary data and writes that to the shared flash at either the DAP offset or bootloader offset.

## Usage

* Open the project configuration menu (`idf.py menuconfig`) go to `Example Configuration` ->
    1. WIFI SSID: WIFI network to which your PC is also connected to.
    2. WIFI Password: WIFI password

* In order to test the OTA demo :
    1. compile and burn the firmware `idf.py -p PORT flash`
    2. run `idf.py -p PORT monitor` and note down the IP assigned to your ESP module. The default port is 80
    3. post something to the server:
        `curl -X POST --data-binary @pipeline.dap http://<your-ip>/upload_dap/`
		or
        `curl -X POST --data-binary @bootlaoder.bin http://<your-ip>/upload_bl/`

Note: An example of generating a real dap file can be found in the standalone-jpeg example:
https://github.com/luxonis/depthai-experiments/tree/master/gen2-spi/standalone-jpeg

Just pass in "save" as the first argument.
`python3 main.py save`
