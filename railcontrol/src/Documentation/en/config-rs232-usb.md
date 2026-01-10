## RS-232 to USB Converter

Some controls offer a RS-232 connector, also known as serial or COM port. Recent computers usually only offer USB connectors. There are converters available from different manufacturers. We recommend to use converters with a FTDI chip. These chips will also be supported with actual drivers from the next generations of operating systems. Other chips usually do not have this support from the next operating system and one has to buy a new converter together.

There are also controls with an internal RS-232 connector and an integrated converter included. Externally the offer an USB connector. These controls are handled identical to a serial connector with extra converter.
Serial ports under Windows (COM-ports)

Under Windows each serial to USB converter gets a fixed COM port. These fixed COM ports usually never change. This means for RailControl that a choosen COM-Port usually never changes and no no further actions are needed.

### Serial ports under Linux

Under Linux all serial to USB converters get a new name at each startup and/or reconnect oft the device. Usually the first connected device is /dev/ttyUSB0 and will be counted up with the next device. If there are more then one devices connected with a serial to USB converter to a Linux computer, the order can change at startup and/or reconnect. If the name changes, Railcontrol can not find the control or worse communicates with the wrong device. In a Linux-terminal with

```
ls /dev/ttyUSB*
```

we get the connected converters:

```
/dev/ttyUSB0   /dev/ttyUSB1
```

The names of the devices regularely swap or one is replaced with /dev/ttyUSB2.

It is possible to add a fixed name or at least a fixed link. Therefore we have to find out some system internal parameters. With

```
lsusb
```

we get a list of all connected USB devices. Beside the usually connected devices there are our two converters (ESU Lokprogrammer and CC-Schnitte) connected in our example (line 2 and 6):

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

Important for us are the IDs. Left to the colon ist the manufacturer ID and right to the colon we find the product ID. Additionally we need the serial number of the devices. We get them with:

```
udevadm info -a -n /dev/ttyUSB0 | grep '{serial}' | head -n1
```

The result is:

```
ATTRS{serial}=="FTF3ZDL5"
```

For the second device we have to repeat the procedure and replace /dev/ttyUSB0 with /devttyUSB1.

With the manufacturer ID, product ID and serial number we can advice udev to add a fix link to these devices. Usually in the path /etc/udev/rules.d or sometimes in /etc/udev/rules (note the missing .d) we create a file with the name 99-usb-serial.rules (if it does not exist).
As content we add something like this (please replace IDs with them of your converters and controls:

```
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", ATTRS{serial}=="FTF3ZDL5", SYMLINK+="LokProgrammer"
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", ATTRS{serial}=="A907FXY2", SYMLINK+="CC-Schnitte"
```

After a reboot of Linux we should see:

```
$ ls /dev/LokProgrammer /dev/CC-Schnitte
/dev/CC-Schnitte  /dev/LokProgrammer
```

RailControl now can use these fix names to communicate with your controls.

