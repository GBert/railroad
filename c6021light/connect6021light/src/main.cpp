// KeyboardDemoSketch
// Demonstrates the i2c communication with a Marklin Keyboard.
// Sends responses to a Keyboard and dumps the received command to the serial console.

// Note that this sketch is not robust against other messages. Recieving engine control messages
// Will likely require a reboot of the uC.

#include <Arduino.h>

#include "Hal.h"
#include "StationCbk.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"

#include "RR32Can/RR32Can.h"
#include "config.h"

// ******** Variables and Constans********
Hal hal;
StationCbk stationCbk;

constexpr const uint8_t myAddr = MarklinI2C::kCentralAddr;

// ******** Code ********

void setup() {
  // Setup Serial
  Serial.begin(115200);  // start serial for output
  Serial.println(F("Connect6021Light Initializing..."));

  // Setup I2C & CAN
  hal.begin(myAddr);

  // Tie callbacks together
  RR32Can::RR32Can.begin(RR32CanUUID, stationCbk, hal);

  Serial.println(F("Ready!"));
}

/**
 * \brief When a message was received, create and send a response message.
 */
void loop() {
  hal.loop();

  // Process I2C
  if (hal.i2cAvailable()) {
    const MarklinI2C::Messages::AccessoryMsg& request = hal.getI2cMessage();
    MarklinI2C::Messages::AccessoryMsg response =
        request.makeResponse();  // TODO: Response should be influenced by the CAN side of things.
    hal.SendI2CMessage(response);

    RR32Can::RR32Can.SendAccessoryPacket(
        request.getTurnoutAddr(), static_cast<RR32Can::TurnoutDirection>(request.getDirection()),
        request.getPower());

    hal.consumeI2cMessage();
  }

  // Process CAN
  // TODO
}
