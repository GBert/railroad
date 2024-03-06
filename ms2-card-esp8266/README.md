Lokkarten Emulator
==================


[alt Lokkarten Emulator](https://raw.githubusercontent.com/GBert/railroad/master/ms2-card-esp8266/ms2-card_sd-card.png)


Auf der Lokkarte befindet sich ein I2C EEPROM (24LC64 64kBit -> 8kByte). Die Schaltung
ersetzt den Speicher durch ein kompatiblen FRAM Baustein. Dadurch entfällt die
zeitkritische I2C Slave Behandlung.

Der Prozessor (ESP8266 oder extern) befüllt den FRAM Baustein mit Lokkarten Daten. Damit
es zu keinen Komplikationen auf dem I2C Bus kommen kann, ist der I2C Bus durch analoge
Schalter (BL1551B - Signal I2C_Select) getrennt.

Der Prozessor signalisiert das Einstecken der Lokkarte (Signal Card) durch Verbindung
der Kontakte am Kartenrand.

Die Platine wurde mit KiCad 7.0.11 erstellt. Die Produktionsdaten wurden mit dem JLCPCB
Fabrication Toolkit generiert.

Die zwei Fehler im DRC Check (Corteyards overlap) können ignoriert werden, da die Bestückung
mit dem SPI EEPROM U10 und C11 alternativ ist.


Detail-Beschreibung
-------------------

Der Kartenwechsel wird bei der MS2 über einen mechanischen Taster erkannt. Bei der MS2 wurden
verschiedene Arten der Kontakte verbaut. Die Platine verwendet einen federnden Testpin, damit
Kartenwechsel simuliert werden kann: https://dirtypcbs.com/store/designer/details/ian/12/dirty-pogo-pins

Power Select: hier wählt man zwiechen Versorgung durch MS2 oder USB.

Schaltregler kann als Ersatz dienen, wenn der Längsregler U6 nicht ausreicht.
Dazu muss U6 aber entfernt werden.

Für C1 ist ein Tantal Kondensator vorgesehen. Da Tantal aber ein Rohstoff aus Krisengebieten
ist, ist ein anderer Kondensator mit niedrigem ESR ratsam. Die Pads sollten genügend Platz bieten,
einen anderen Kondensator einzusetzen, wie z.B. Reichelts "VF 100/6,3 P-C" oder "VF 100/16 P-D".

Silabs CP2102 wurde für die Seriell-USB Anbindung gewählt, weil dieser ein guter Kompromiss
zwischen Stabilität, Unterstützung und Preis ist.

Q1/Q2 und R8/R9 dienen dazu, das Update der ESP8266 ohne Eingriff zu ermöglichen.

U3 und U4 trennen den I2C Bus von der MS2, damit es keine Störungen des MS2 I2C-Busses gibt.
I2C Select schaltet entweder den ESP8266 oder alternativ einen anderen I2C Master
für den Zugriff auf das FRAM hinzu.

U20 dient dazu, einen Kartenwechsel der MS2 zu signalisieren.

Das MS2 I2C Bus Clock Signal wird zusätzlich auch noch über Q1 und ein RC Glied an den ADC
des ESP8266 Moduls angeschlossen, um den Zeitpunkt des letzten I2C Bus Zugriffs zu messen.

TODOs
-----

- CP2102 Test
- I2C intern Test
- I2C extern Test
- ESP Programmierung Test
- SD Kartenanbindung Test
- Allternative zur SD-Karte - SPI EEPROM

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

https://www.mikrocontroller.net/articles/MMC-_und_SD-Karten

Pull-up D0
SPI-Mode Pull-up ungenutzter Ports (TODO)
470 nH Vdd (TODO)


Raspberry Pi I2C
----------------

|Function  | GPIO   | Pin    |      |
|----------|--------|--------|------|
|SDA       | GPIO2  | Pin 3  |      |
|SCL       | GPIO3  | Pin 5  |      |

