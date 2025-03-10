#ifndef __HAL__HALBASE_H__
#define __HAL__HALBASE_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/callback/TxCbk.h"

namespace hal {

/*
 * \brief Class HalBase
 */
class HalBase : public RR32Can::callback::TxCbk {
 public:
  void begin(uint8_t i2caddr) { i2cLocalAddr = i2caddr; }

  bool i2cAvailable() const { return i2cMessageReceived; }
  const volatile MarklinI2C::Messages::AccessoryMsg& getI2cMessage() const { return i2cMsg; }
  virtual void consumeI2cMessage() { i2cMessageReceived = false; }

  MarklinI2C::Messages::AccessoryMsg prepareI2cMessage();

  /**
   * \brief Send a given message over I2C.
   */
  virtual void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) = 0;

 protected:
  static uint8_t i2cLocalAddr;

  /// Whether a message was received (i.e., lastMsg has valid contents).
  static volatile bool i2cMessageReceived;

  /// The last message that was received over i2c.
  static volatile MarklinI2C::Messages::AccessoryMsg i2cMsg;
};

}  // namespace hal

#endif  // __HAL__HALBASE_H__
