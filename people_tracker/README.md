# People tracker

This demo application recieves tracklets from the [Gen2 People tracker](https://github.com/luxonis/depthai-experiments/tree/master/gen2-people-tracker) script over SPI.

## Demo


## Run this example
First run the [Gen2 People tracker](https://github.com/luxonis/depthai-experiments/tree/master/gen2-people-tracker) script with `-spi` argument
```
python3 ~/depthai-experiments/gen2-people-tracker/main.py -spi
```

Then, flash the application to the ESP32 and open the monitor
```
idf.py -p PORT flash monitor
```