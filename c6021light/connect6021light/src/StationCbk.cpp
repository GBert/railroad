#include "StationCbk.h"

#include "Arduino.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/Constants.h"

void StationCbk::begin(Hal& hal) { this->hal = &hal; }

void StationCbk::OnAccessoryPacket(RR32Can::TurnoutPacket& packet) {
  // TODO: Send via I2C
  Serial.print(F("Got an Accessory packet!"));

  if (hal == nullptr) {
    return;
  }

  if ((packet.locid & RR32Can::kMMAccessoryAddrStart) != RR32Can::kMMAccessoryAddrStart) {
    // Not an MM2 packet
    return;
  }

  uint16_t turnoutAddr = packet.locid & 0x03FF;
  if (turnoutAddr > 0xFF) {
    // Addr too large for the i2c bus.
    return;
  }

  // Convert to i2c confirmation packet
  MarklinI2C::Messages::AccessoryMsg i2cMsg = hal->prepareI2cMessage();

  i2cMsg.setTurnoutAddr(turnoutAddr);
  i2cMsg.setPower(packet.power);
  i2cMsg.setDirection(packet.position);

  hal->SendI2CMessage(i2cMsg);
}
