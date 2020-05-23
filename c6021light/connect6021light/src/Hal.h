#ifndef __HAL_H__
#define __HAL_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/StationTxCbk.h"

/*
 * \brief Class Hal
 */
class Hal : public RR32Can::StationTxCbk {
 public:
  ///
  void begin(uint8_t i2caddr) {
    i2cLocalAddr = i2caddr;
    beginI2c();
    beginCan();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() {
    loopI2c();
    loopCan();
  }

  bool i2cAvailable() const { return i2cMessageReceived; }
  const MarklinI2C::Messages::AccessoryMsg& getI2cMessage() const { return i2cMsg; }
  void consumeI2cMessage() { i2cMessageReceived = false; }

  /**
   * \brief Send a given message over I2C.
   */
  void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg);

 private:
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginI2c();
  void beginCan();

  void loopCan();
  void loopI2c();

  static uint8_t i2cLocalAddr;

  static void receiveEvent(int howMany);

  /// Whether a message was received (i.e., lastMsg has valid contents).
  static bool i2cMessageReceived;

  /// The last message that was received over i2c.
  static MarklinI2C::Messages::AccessoryMsg i2cMsg;
};

#endif  // __HAL_H__
