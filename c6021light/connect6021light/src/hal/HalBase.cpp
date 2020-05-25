#include "hal/HalBase.h"

namespace hal {

uint8_t HalBase::i2cLocalAddr;
bool HalBase::i2cMessageReceived;
MarklinI2C::Messages::AccessoryMsg HalBase::i2cMsg;

MarklinI2C::Messages::AccessoryMsg HalBase::prepareI2cMessage() {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.source = i2cLocalAddr;
  return msg;
}

}  // namespace hal
