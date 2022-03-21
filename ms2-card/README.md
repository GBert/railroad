ESP8266
-------

Tasmota Tests
-------------

git clone https://github.com/arendst/Tasmota.git
cd Tasmota
git tag
pio run

{"NAME":"ESP-12F","GPIO":[1,1,1,1,1,1,0,0,1,1,1,1,1,1],"FLAG":0,"BASE":18}


user_config_override.h
```
// Select none or only one of the below defines USE_RULES or USE_SCRIPT
#ifdef USE_RULES 
#undef USE_RULES                                // Add support for rules (+8k code)
#endif

#ifndef USE_SCRIPT
#define USE_SCRIPT
#endif
#define SDCARD_DIR //Not essential I think
#define USE_SCRIPT_FATFS_EXT //Not essential I think
#define USE_SCRIPT_FATFS //Not essential I think
#define USE_UFILESYS
#define USE_SDCARD
#define SDCARD_CS_PIN 4 //Not strictly necessary since the same #define happens in xdrv_50_filesystem.ino

//Further down I define SPI

#define USE_SPI
```


Flashing
--------
```
esptool.py --port /dev/ttyUSB1 erase_flash 
esptool.py --port /dev/ttyUSB1 --baud 460800 write_flash --flash_size=detect 0 ./build_output/firmware/tasmota.bin
```



Raspberry Pi I2C Slave
----------------------

|Function  | GPIO   | Pin    |      |
|----------|--------|--------|------|
|SDA       | GPIO2  | Pin 3  |      |
|SCL       | GPIO3  | Pin 5  |      |
|SDA Slave | GPIO10 | Pin 19 | used |
|SCL Slave | GPIO11 | Pin 23 | used |



PI Pico
------------------------------------

I2C Slave

| GPIO | Function |       |
|------|----------|-------|
| GP4  | SDA      | PIN 6 |
| GP5  | SCL      | PIN 7 |


https://draeger-it.blog/maker-pi-pico-2-sensordaten-auf-einer-sd-card-speichern/


| GPIO   | SD Mode | SPI Mode   |
|--------|---------|------------|
| GP10   | CLK     | SCK        |
| GP11   | CMD     | SDI / MOSI |
| GP12   | DAT0	   | SDO / MISO |
| GP13   | DAT1	   | X          |
| GP14   | DAT2	   | X          |
| GP15   | CD/DAT3 | CSn        |
