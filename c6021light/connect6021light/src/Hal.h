#ifndef __HAL_H__
#define __HAL_H__

#include "RR32Can/StationTxCbk.h"

/*
 * \brief Class Hal
 */
class Hal : public RR32Can::StationTxCbk {
 public:
  ///
  void begin() {
    beginI2c();
    beginCan();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() {
    loopI2c();
    loopCan();
  }

 private:
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginI2c();
  void beginCan();

  void loopCan();
  void loopI2c();
};

#endif  // __HAL_H__
