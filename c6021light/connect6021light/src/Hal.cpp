#include "Hal.h"

#include <CAN.h>
#include <Wire.h>
#include "RR32Can/RR32Can.h"

uint8_t Hal::i2cLocalAddr;
bool Hal::i2cMessageReceived;
MarklinI2C::Messages::AccessoryMsg Hal::i2cMsg;

/**
 * \brief Callback when a message is received.
 */
void Hal::receiveEvent(int howMany) {
  if (howMany != 2) {
    Serial.println(F("I2C message with incorrect number of bytes."));
    Wire.flush();
  }

  if (!Hal::i2cMessageReceived) {
    Hal::i2cMsg.destination = Hal::i2cLocalAddr;
    Hal::i2cMsg.source = Wire.read();
    Hal::i2cMsg.data = Wire.read();
    Serial.println(F("I2C Message received."));
    Hal::i2cMsg.print();
    Hal::i2cMessageReceived = true;
  } else {
    Serial.println(F("I2C RX Buffer full, lost message."));
  }
}

void Hal::beginI2c() {
  i2cMessageReceived = false;
  Wire.begin(i2cLocalAddr);
  Wire.onReceive(Hal::receiveEvent);
}

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
    Serial.print(F("Received "));

    if (CAN.packetExtended()) {
      Serial.print(F("extended "));
    }

    if (CAN.packetRtr()) {
      // Remote transmission request, packet contains no data
      Serial.print(F("RTR "));
    }

    Serial.print(F("packet with id 0x"));
    long packetId = CAN.packetId();
    Serial.print(packetId, HEX);
    RR32Can::Identifier rr32id = RR32Can::Identifier::GetIdentifier(packetId);
    RR32Can::Data data;
    data.dlc = packetSize;

    if (CAN.packetRtr()) {
      Serial.print(F(" and requested length "));
      Serial.println(CAN.packetDlc());
    } else {
      Serial.print(F(" and length "));
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

void Hal::SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) {
  Wire.beginTransmission(msg.destination);
  Wire.write(msg.source);
  Wire.write(msg.data);
  Wire.endTransmission();
  Serial.println(F("I2C message sent."));
}
