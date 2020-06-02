#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include "hal/HalBase.h"

namespace hal {

/*
 * \brief Class LibOpencm3Hal
 */
class LibOpencm3Hal : public HalBase {
 public:
  ///
  void begin(uint8_t i2caddr) {
    HalBase::begin(i2caddr);
    beginClock();
    beginSerial();
    beginI2c();
    beginCan();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() { loopCan(); }

  void consumeI2cMessage() {
    bytesRead = 0;
    HalBase::consumeI2cMessage();
  }

  /**
   * \brief Send a given message over I2C.
   */
  void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) override;

  static void i2cEvInt(void);

  static volatile bool canAvailable;

 private:
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginClock();
  void beginSerial();
  void beginI2c();
  void beginCan();

  void loopCan();

  static volatile uint_fast8_t bytesRead;
  static volatile uint_fast8_t bytesSent;

  static volatile MarklinI2C::Messages::AccessoryMsg i2cTxMsg;
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
