// KeyboardDemoSketch
// Demonstrates the i2c communication with a Marklin Keyboard.
// Sends responses to a Keyboard and dumps the received command to the serial console.

// Note that this sketch is not robust against other messages. Recieving engine control messages
// Will likely require a reboot of the uC.

#include <Arduino.h>
#include <Wire.h>

#include "MarklinI2C/Messages/AccessoryMsg.h"

constexpr const uint8_t myAddr = MarklinI2C::kCentralAddr;

/// The last message that was received.
MarklinI2C::Messages::AccessoryMsg lastMsg;

/// Whether a message was received (i.e., lastMsg has valid contents).
bool messageReceived;

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
  messageReceived = false;
  Wire.begin(myAddr);            // join i2c bus with address #8
  Wire.onReceive(receiveEvent);  // register event
  Serial.begin(115200);          // start serial for output
  Serial.println(F("Connect6021Light ready."));
}

/**
 * \brief When a message was received, create and send a response message.
 */
void loop() {
  if (messageReceived) {
    // Craft a response
    MarklinI2C::Messages::AccessoryMsg response;
    response.destination = lastMsg.source >> 1;  // guesswork: shift by 1. Observation: Keyboard
                                                 // that sent with 20 gets its response on 10.
    response.source =
        lastMsg.destination
        << 1;  // guesswork: shift by 1. Observation: Central that received on 0x7F sends from 0xFE
    response.data = lastMsg.data;
    SendMessage(response);
    messageReceived = false;
  }
}
