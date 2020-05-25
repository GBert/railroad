#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include "Hal/HalBase.h"

namespace hal {

/*
 * \brief Class LibOpencm3Hal
 */
class LibOpencm3Hal : public HalBase {
 public:
  ///
  void begin(uint8_t i2caddr) {
    HalBase::begin(i2caddr);
    beginSerial();
    beginI2c();
    beginCan();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() {
    loopI2c();
    loopCan();
  }

  /**
   * \brief Send a given message over I2C.
   */
  void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) override;

 private:
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginSerial();
  void beginI2c();
  void beginCan();

  void loopCan();
  void loopI2c();

  static void receiveEvent(int howMany);
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
