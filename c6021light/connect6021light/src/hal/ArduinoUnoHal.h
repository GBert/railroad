#ifndef __HAL_H__
#define __HAL_H__

#include <Arduino.h>

#include "hal/HalBase.h"

namespace hal {

/*
 * \brief Class ArduinoUnoHal
 */
class ArduinoUnoHal : public HalBase {
 public:
  ///
  void begin(uint8_t i2caddr) {
    HalBase::begin(i2caddr);
    Serial.begin(115200);
    beginI2c();
    beginCan();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() {
    loopI2c();
    loopCan();
  }

  virtual void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) override;

 private:
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginI2c();
  void beginCan();

  void loopCan();
  void loopI2c();

  static void receiveEvent(int howMany);
};

}  // namespace hal

#endif  // __HAL_H__
