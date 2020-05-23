#ifndef __MARKLINI2C__MESSAGES__ACCESSORYMSG_H__
#define __MARKLINI2C__MESSAGES__ACCESSORYMSG_H__

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

#include "MarklinI2C/Constants.h"

namespace MarklinI2C {
namespace Messages {

/*
 * \brief Class AccessoryMsg
 */
class AccessoryMsg {
 public:
  AccessoryMsg(){};
  AccessoryMsg(uint8_t destination, uint8_t source, uint8_t data)
      : destination(destination), source(source), data(data){};

  // uint8_t getSource() const { return source; }

  uint8_t getSender() const { return (source & 0b00011110) >> 1; }

  // uint8_t getDestination() const { return destination; }

  /// Obtian the de-masked decoder output address.

  uint8_t getDecoderOutput() const;

  /**
   * \brief Obtain the complete turnout address.
   *
   * The result is 0-based. Pressing a button for Turnout 1
   * on the first Keyboard is Address 0.
   */
  uint8_t getTurnoutAddr() const;

  uint8_t getDirection() const { return data & kDataDirMask; }
  uint8_t getPower() const { return (data & kDataPowerMask) >> 3; }

  void print() const;

  uint8_t destination = 0;
  uint8_t source = 0;
  uint8_t data = 0;
};

}  // namespace Messages
}  // namespace MarklinI2C

#endif  // __MARKLINI2C__MESSAGES__ACCESSORYMSG_H__
