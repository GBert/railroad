#include "StationCbk.h"

#include "Arduino.h"

void StationCbk::OnAccessoryPacket(RR32Can::TurnoutPacket& packet) {
  // TODO: Send via I2C
  Serial.print(F("Got an Accessory packet!"));
}
