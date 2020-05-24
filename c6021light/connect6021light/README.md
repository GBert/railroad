# connect6021 light

Project to equip an Arduino Uno + CANdiy shield to serve as a drop-in-replacement for a 6021
Central Unit for connecting Keyboards (only!) to CAN.

## Notes on Supply Power

The Keyboard operates on 8V. Note that its power draw is quite significant at about 120mA for a
single device.

The I2C bus operates at 5V.

## I2C Messages

Devices communicate over I2C. The system is multi-master, i.e., any device that has data to send
as as a master. Read from slave devices is not used.

The message format for Keyboards (Accessory commands) is as follows:

receiver sender data

* receiver is the I2C slave address the message is sent to. Thisi s not sent as a separate data
  byte but as the address byte of the I2C communication.
* sender is the I2C slave address under which the originator of the message can be reached.
* data The data byte, containing the action to be performed.

### I2C Device Addresses

Note that I2C addresses are 7 Bit. However, the above mentioned fields are all 8 bit. In practice,
this results in the fact that the receiver address and sender address are shifted by 1 bit. In the
following, all addresses are given as their I2C slave address repesentation (i.e., the value
observed in the receiver field).

* The Central Unit uses the slave address 0x7F.
* Keyboard 0 uses a slave address of 0x10. The Keyboard address is derived from the DIP switches on
  the back.

### I2C Data Byte

The data byte is constructed as follows:

[0 0 Decoder_MSB Decoder_LSB Power Output_MSB Output_LSB Direction]

Addressing on decoders on I2C is centered around the original approach, where a decoder had one
decoder address (set with DIP switches) and eight outputs. The KEyboard Address together with
the Decoder_* bits identify a decoder. The Output_* bits together with the Direction bit identify
one output of the decoder.

Modern Systems use individually addressable decoders (Keyboard Address + Decoder_* + Output_*) and
treat the direction separately.

## Keyboard Message flow

The Keyboard does not need any initialization. As soon as 8V are applied to one of the power pins
and the I2C lines are pulled up to 5V (a 10k resistor on each line does the job) it starts up and
the turnout indicators set at the last shutdown light up.
 
When a button is pressed, a message is sent to the Central Unit. When no Device with the address
of the Central Unit is present on the bus, the `receiver` byte will not be Acknowledged. In this
case, the Keyboard will abort transmission of the remaining bytes. It will then try resend the
message immediately and indefinitely, possibly blocking the bus.

If the message is received (i.e., a device with the address of the Central Unit is present on the
bus and ACKs the bytes), the Keyboard expects a response. If the response is not received, it
resends its original request indefinitely about once per second.

If no response is received, the indicators on the Keyboard don't change. Also, pressing additional
buttons has no effect. The Keyboard is stuck processing the original request. If a response is
received even when no corresponding request was sent, the indicators always change and no further
processing takes place on the Keyboard.

### Power On/Off

When a button is pressed, a "Power ON" message is sent. When a button is released, a "Power OFF"
message is sent. However, due to the Decoder-Based addressing scheme, the "Power Off" message is
not addressed towards a specific turnout but towards the decoder Address, i.e., all 8 outputs are
considered to be unpowered.