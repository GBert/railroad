Lokkarten Emulator
------------------

Auf der Lokkarte befindet sich ein I2C EEPROM (24LC64 64kBit -> 8kByte). Die Schaltung
ersetzt den Speicher durch ein kompatiblen FRAM Baustein. Dadurch entfällt die
zeitkritische I2C Slave Behandlung.

Der Prozessor (ESP8266 oder extern) befüllt den FRAM Baustein mit Lokkarten Daten. Damit
es zu keinen Komplikationen auf dem I2C Bus kommen kann, ist der I2C Bus durch analoge
Schalter (BL1551B - Signal I2C_Select) getrennt.

Der Prozessor signalisiert das Einstecken der Lokkarte (Signal Card) durch Verbindung
der Kontakte am Kartenrand.


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

SD-CARD
-------
https://github.com/arendst/Tasmota/discussions/13938


Raspberry Pi I2C Slave
----------------------

|Function  | GPIO   | Pin    |      |
|----------|--------|--------|------|
|SDA       | GPIO2  | Pin 3  |      |
|SCL       | GPIO3  | Pin 5  |      |
|SDA Slave | GPIO10 | Pin 19 | used |
|SCL Slave | GPIO11 | Pin 23 | used |

