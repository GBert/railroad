## RS-232 zu USB Konverter

Manche Zentralen verfügen über einen RS-232-Anschluss, auch bekannt als Serieller Anschluss oder COM-Ports. Aktuelle Computer verfügen hingegen meist nur über USB-Anschlüsse. Passende Konverter sind zu günstigen Konditionen erhältlich. Dabei ist darauf zu achten, dass vorzugsweise Konverter mit einem Chip von FTDI zum Einsatz kommen. Diese Chips werden über mehrere Betriebssystem-Generationen mit passenden Treibern versorgt, was bei vielen anderen Herstellern üblicherweise nicht oder nicht immer der Fall ist.

Auch gibt es Zentralen, die intern einen RS-232-Anschluss und einen RS-232 zu USB Konverter Chip integriert haben. Nach aussen ist nur ein USB-Anschluss sichtbar. Diese Zentralen verhalten sich gleich wie wenn sie mit einem externen Konverter angeschlossen wären.

### Serielle Anschlüsse unter Windows (COM-Ports)

Windows ordnet einem RS-232 zu USB Konverter einen festen COM-Port zu. Diese Zuordnung ist auch nach einem Neustart von Windows und nach dem Entfernen und Wiedereinstecke des Gerätes immer noch gleich. Für RailControl bedeutet das, dass ein ausgewählter COM-Port üblicherweise bestehen bleibt und keine weiteren Schritte nötig sind.

### Serielle Anschlüsse unter Linux

Unter Linux werden die RS-232 zu USB Konverter nach jedem Neustart von Linux sowie bei jedem Neuverbinden des Gerätes unter einem neuen Namen zugeordnet. Üblicherweise startet dies bei /dev/ttyUSB0 und wird dann hochgezählt. Werden mehrere Geräte mit einem seriellen Anschluss an Linux angeschlossen hat das zur Folge, dass die Reihenfolge nicht zwingend immer gleich sein muss. Wird die Namensgebung durch Linux geändert findet RailControl die entsprechenden Zentralen nicht mehr oder spricht mit einem falschen Gerät. Auf einer Konsole erhalten wir mit

```
ls /dev/ttyUSB*
```

die angeschlossenen Namen der angeschlossenen Konverter:

```
/dev/ttyUSB0   /dev/ttyUSB1
```

Die Namen der Geräte werden regelmässig vertauscht oder einer der beiden gar mit /dev/ttyUSB2 ersetzt.

Mit etwas Aufwand ist es jedoch möglich den RS-232 zu USB Konvertern einen festen Namen oder zumindest einen fixen Link zu erteilen.

Dazu müssen wir herausfinden, wie ein Konverter aus Systemsicht angesprochen wird. Mit

```
lsusb
```

erhalten wir eine Liste mit den angeschlossenen USB-Geräten. In unserem Beispiel sind neben weiteren Geräten zwei Konverter (ESU Lokprogrammer und CC-Schnitte) angeschlossen, die regelmässig mit vertauschten Gerätenamen eingebunden werden (Zeile 2 und 6):

```
Bus 002 Device 003: ID 0408:03ba Quanta Computer, Inc. 
Bus 002 Device 021: ID 0403:6001 Future Technology Devices International, Ltd FT232 USB-Serial (UART) IC
Bus 002 Device 002: ID 8087:0020 Intel Corp. Integrated Rate Matching Hub
Bus 002 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 001 Device 008: ID 0461:4d17 Primax Electronics, Ltd Optical Mouse
Bus 001 Device 007: ID 0403:6001 Future Technology Devices International, Ltd FT232 USB-Serial (UART) IC
Bus 001 Device 002: ID 8087:0020 Intel Corp. Integrated Rate Matching Hub
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
```

Wichtig für uns sind die IDs, welche aus der Hersteller-Nummer vor dem Doppelpunkt und der Produkte-Nummer nach dem Doppelpunkt besteht. Weiter benötigen wir die Seriennummer der Geräte. Diese erhalten wir mit:

```
udevadm info -a -n /dev/ttyUSB0 | grep '{serial}' | head -n1
```

Das Resultat ist folgendes:

```
ATTRS{serial}=="FTF3ZDL5"
```

Für das zweite Gerät muss das ganze wiederholt werden, jedoch muss /dev/ttyUSB0 mit /devttyUSB1 ersetzt werden.

Die erhaltenen Angaben Hersteller-, Produkt- und Serien-Nummer können wir nun in eine neue Datei eintragen. Diese Datei ist bei den meisten Distriutionen im Pfad /etc/udev/rules.d oder in seltenen Fällen unter /etc/udev/rules anzulegen und sollte den Namen 99-usb-serial.rules erhalten.
In den Inhalt der Datei kommen unsere Angaben in der folgenden Form:

```
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", ATTRS{serial}=="FTF3ZDL5", SYMLINK+="LokProgrammer"
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", ATTRS{serial}=="A907FXY2", SYMLINK+="CC-Schnitte"
```

Nach einem Neustart des Systems sollten die folgenden Angaben ersichtlich sein:

```
$ ls /dev/LokProgrammer /dev/CC-Schnitte
/dev/CC-Schnitte  /dev/LokProgrammer
```

RailControl kann dann diese fixen Namen (/dev/LokProgrammer und /dev/CC-Schnitte) verwenden um die Zentralen zuverlässig ansprechen zu können.

