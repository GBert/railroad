// KeyboardDemoSketch
// Demonstrates the i2c communication with a Marklin Keyboard.
// Sends responses to a Keyboard and dumps the received command to the serial console.

// Note that this sketch is not robust against other messages. Recieving engine control messages
// Will likely require a reboot of the uC.

#include <Arduino.h>
#include <Wire.h>

#include <CAN.h>

#include "MarklinI2C/Messages/AccessoryMsg.h"

// ******** CAN Stuff ********

// ******** I2C Stuff ********
constexpr const uint8_t myAddr = MarklinI2C::kCentralAddr;

/// The last message that was received.
MarklinI2C::Messages::AccessoryMsg lastMsg;

/// Whether a message was received (i.e., lastMsg has valid contents).
bool messageReceived;

// ******** Code ********

/**
 * \brief Send a given message over I2C.
 */
void SendMessage(const MarklinI2C::Messages::AccessoryMsg& msg) {
  Wire.beginTransmission(msg.destination);
  Wire.write(msg.source);
  Wire.write(msg.data);
  Wire.endTransmission();
  Serial.println(F("Response sent."));
}

/**
 * \brief Callback when a message is received.
 */
void receiveEvent(int howMany) {
  if (!messageReceived) {
    lastMsg.destination = myAddr;
    lastMsg.source = Wire.read();
    lastMsg.data = Wire.read();
    Serial.println(F("Message received."));
    lastMsg.print();
    messageReceived = true;
  } else {
    Serial.println(F("Buffer full, lost message."));
  }
}

void setup() {
  // Setup Serial
  Serial.begin(115200);  // start serial for output
  Serial.println(F("Connect6021Light Initializing..."));

  // Setup I2C
  messageReceived = false;
  Wire.begin(myAddr);            // join i2c bus with address #8
  Wire.onReceive(receiveEvent);  // register event

  // Setup CAN
  Serial.print(F("Init Can: "));
  if (!CAN.begin(250E3)) {
    Serial.println(F(" failed."));
    while (1);
  } else {
    Serial.println(F(" success."));
  }

  Serial.println(F("Ready!"));
}

/**
 * \brief When a message was received, create and send a response message.
 */
void loop() {
  // Process I2C
  if (messageReceived) {
    // Craft a response
    MarklinI2C::Messages::AccessoryMsg response = lastMsg.makeResponse();
    SendMessage(response);
    messageReceived = false;
  }

  // Process CAN
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
    Serial.print(CAN.packetId(), HEX);

    if (CAN.packetRtr()) {
      Serial.print(" and requested length ");
      Serial.println(CAN.packetDlc());
    } else {
      Serial.print(" and length ");
      Serial.println(packetSize);

      // only print packet data for non-RTR packets
      while (CAN.available()) {
        Serial.print((char)CAN.read());
      }
      Serial.println();
    }

    Serial.println();
  }
}
