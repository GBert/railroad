## Configuration of Fleischmann Twin-Center

To configure a Fleischmann Twin-Center there has to be selected the serial port.

Especially under Linux one should consider the page [RS-232 to USB converter](#rs-232-to-usb-converter).

There also has to be configured the number of S88 feedback modules, that are connected to a Twin-Center. At most there can be connected 104 modules with 8 input pins. Modules with 16 input pins have to be configured as two modules.

Because of the principles of the protocol this control does not recognize very short feedback pulses. Therefore it is recommended to use other possibilities for feedbacks to realize an automatic operation.

