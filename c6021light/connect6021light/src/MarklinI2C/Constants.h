#ifndef __MARKLINI2C__CONSTANTS_H__
#define __MARKLINI2C__CONSTANTS_H__

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

namespace MarklinI2C {

constexpr const uint8_t kCentralAddr = 0x7F;

constexpr const uint8_t kSenderAddrMask = 0b00011110;

constexpr const uint8_t kDataDirMask = 0x01;
constexpr const uint8_t kDataDirRed = 0x00;
constexpr const uint8_t kDataDirGreen = 0x01;

constexpr const uint8_t kDataPowerMask = 0b00001000;

constexpr const uint8_t kDataLowerAddrMask = 0b00000110;
constexpr const uint8_t kDataUpperAddrMask = 0b00110000;

}  // namespace MarklinI2C

#endif  // __MARKLINI2C__CONSTANTS_H__
