#include "Hal.h"

#include <CAN.h>
#include "RR32Can/RR32Can.h"

void Hal::beginI2c() {}

void Hal::beginCan() {
  Serial.print(F("Init Can: "));
  if (!CAN.begin(250E3)) {
    Serial.println(F(" failed."));
    while (1)
      ;
  } else {
    Serial.println(F(" success."));
  }
}

void Hal::loopI2c() {}

void Hal::loopCan() {
  int packetSize = CAN.parsePacket();

  if (packetSize > 0) {
    // received a packet
    Serial.print("Received ");

    if (CAN.packetExtended()) {
      Serial.print("extended ");
    }

    if (CAN.packetRtr()) {
      // Remote transmission request, packet contains no data
      Serial.print("RTR ");
    }

    Serial.print("packet with id 0x");
    long packetId = CAN.packetId();
    Serial.print(packetId, HEX);
    RR32Can::Identifier rr32id = RR32Can::Identifier::GetIdentifier(packetId);
    RR32Can::Data data;
    data.dlc = packetSize;

    if (CAN.packetRtr()) {
      Serial.print(" and requested length ");
      Serial.println(CAN.packetDlc());
    } else {
      Serial.print(" and length ");
      Serial.println(packetSize);

      // only print packet data for non-RTR packets
      uint8_t i = 0;
      while (CAN.available()) {
        data.data[i] = CAN.read();
        Serial.print(' ');
        Serial.print(data.data[i], HEX);
        ++i;
      }
      Serial.println();
      RR32Can::RR32Can.HandlePacket(rr32id, data);
    }

    Serial.println();
  }
}

void Hal::SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) {
  CAN.beginExtendedPacket(id.makeIdentifier(), data.dlc, false);
  for (uint8_t i = 0; i < data.dlc; ++i) {
    CAN.write(data.data[i]);
  }
  CAN.endPacket();
}