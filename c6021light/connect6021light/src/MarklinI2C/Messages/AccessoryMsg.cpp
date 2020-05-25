#include "MarklinI2C/Messages/AccessoryMsg.h"

#include "RR32Can/StlAdapter.h"

namespace MarklinI2C {
namespace Messages {

uint8_t AccessoryMsg::getDecoderOutput() const {
  uint8_t lowerBits = data & kDataLowerAddrMask;
  lowerBits >>= 1;

  uint8_t upperBits = data & kDataUpperAddrMask;
  upperBits >>= 2;

  uint8_t addr = upperBits | lowerBits;
  return addr;
}

uint8_t AccessoryMsg::getTurnoutAddr() const {
  uint8_t addr = 0;
  addr |= getSender();
  addr <<= 4;
  addr |= getDecoderOutput();
  return addr;
}

void AccessoryMsg::setTurnoutAddr(uint8_t addr) {
  {
    this->destination = addr & 0xF0;
    this->destination >>= 3;
    this->destination &= 0b00100000;
  }
  {
    uint8_t decoderAddrPart = addr & 0x0C;
    decoderAddrPart <<= 2;
    decoderAddrPart &= kDataUpperAddrMask;
    this->data |= decoderAddrPart;
  }
  {
    uint8_t decoderOutput = addr & 0x03;
    decoderOutput <<= 1;
    decoderOutput &= kDataLowerAddrMask;
    this->data |= decoderOutput;
  }
}

AccessoryMsg AccessoryMsg::makeResponse() const {
  AccessoryMsg response;
  response.destination = source >> 1;  // Shift Source address to destination space.
  response.source = destination << 1;  // Shift destination address to source space.
  response.data = data;
  return response;
}

void AccessoryMsg::print() const {
  Serial.print('[');
  Serial.print(destination, BIN);
  Serial.print(' ');
  Serial.print(source, BIN);
  Serial.print(' ');
  Serial.print(data, BIN);
  Serial.print(']');

  // Sender
  Serial.print(F(" Keyboard: "));
  Serial.print(getSender(), DEC);

  Serial.print(F(" Decoder: "));
  Serial.print(getDecoderOutput(), DEC);

  Serial.print(F(" (Turnout Addr: "));
  Serial.print(getTurnoutAddr(), DEC);

  Serial.print(F(") Direction: "));
  switch (getDirection()) {
    case kDataDirRed:
      Serial.print(F("RED  "));
      break;
    case kDataDirGreen:
      Serial.print(F("GREEN"));
      break;
    default:
      Serial.print(F("WTF"));
      break;
  }

  Serial.print(F(" Power: "));
  switch (getPower()) {
    case 0:
      Serial.print(F("OFF"));
      break;
    case 1:
      Serial.print(F("ON "));
      break;
    default:
      Serial.print(F("WTF"));
      break;
  }

  Serial.println();
}

}  // namespace Messages
}  // namespace MarklinI2C
