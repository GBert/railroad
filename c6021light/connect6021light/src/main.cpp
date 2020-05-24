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

uint8_t lastPowerOnTurnoutAddr;
uint8_t lastPowerOnDirection;

constexpr const uint8_t myAddr = MarklinI2C::kCentralAddr;

// ******** Code ********

void setup() {
  // Setup Serial
  Serial.begin(115200);  // start serial for output
  Serial.println(F("Connect6021Light Initializing..."));

  // Setup I2C & CAN
  hal.begin(myAddr);

  // Tie callbacks together
  stationCbk.begin(hal);
  RR32Can::RR32Can.begin(RR32CanUUID, stationCbk, hal);

  Serial.println(F("Ready!"));
}

/**
 * \brief Two addresses are on the same decoder, if they match apart from
 * the lowest two bits.
 */
bool sameDecoder(uint8_t left, uint8_t right) {
  constexpr const uint8_t mask = 0xFC;
  return (left & mask) == (right & mask);
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

    // If this is a power ON packet: Send directly to CAN
    if (request.getPower()) {
      lastPowerOnDirection = request.getDirection();
      lastPowerOnTurnoutAddr = request.getTurnoutAddr();
      RR32Can::RR32Can.SendAccessoryPacket(
          lastPowerOnTurnoutAddr, static_cast<RR32Can::TurnoutDirection>(request.getDirection()),
          request.getPower());
    } else {
      // On I2C, for a Power OFF message, the two lowest bits (decoder output channel) are always 0,
      // regardless of the actual turnout address to be switched off. For safety, translate this to
      // tuning off all addresses of the respective decoder.
      //
      // Note that we store the last direction where power was applied.
      // The CAN side interprets a "Power Off" as "Flip the switch" anyways.
      uint8_t i2cAddr = request.getTurnoutAddr();
      if (sameDecoder(i2cAddr, lastPowerOnTurnoutAddr)) {
        RR32Can::RR32Can.SendAccessoryPacket(
            lastPowerOnTurnoutAddr, static_cast<RR32Can::TurnoutDirection>(lastPowerOnDirection),
            request.getPower());
      }
    }

    hal.consumeI2cMessage();  // Free the input buffer
  }

  // Process CAN: Done through callbacks.
}
