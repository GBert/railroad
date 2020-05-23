// KeyboardDemoSketch
// Demonstrates the i2c communication with a Marklin Keyboard.
// Sends responses to a Keyboard and dumps the received command to the serial console.

// Note that this sketch is not robust against other messages. Recieving engine control messages
// Will likely require a reboot of the uC.

#include <Arduino.h>
#include <Wire.h>

#include "Hal.h"
#include "StationCbk.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"

#include "RR32Can/RR32Can.h"
#include "config.h"

// ******** CAN Stuff ********
Hal hal;
StationCbk stationCbk;

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
  hal.begin();

  RR32Can::RR32Can.begin(RR32CanUUID, stationCbk, hal);

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
  hal.loop();
}
