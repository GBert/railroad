// KeyboardDemoSketch
// Demonstrates the i2c communication with a Marklin Keyboard.
// Sends responses to a Keyboard and dumps the received command to the serial console.

// Note that this sketch is not robust against other messages. Recieving engine control messages
// Will likely require a reboot of the uC.

// This example code is in the public domain.


#include <Wire.h>

constexpr const uint8_t kCentralAddr = 0x7F;
constexpr const uint8_t myAddr = kCentralAddr;

constexpr const uint8_t kSenderAddrMask = 0b00011110;

constexpr const uint8_t kDataDirMask = 0x01;
constexpr const uint8_t kDataDirRed = 0x00;
constexpr const uint8_t kDataDirGreen = 0x01;

constexpr const uint8_t kDataPowerMask = 0b00001000;

constexpr const uint8_t kDataLowerAddrMask = 0b00000110;
constexpr const uint8_t kDataUpperAddrMask = 0b00110000;


/**
 * \brief Struct to hold a Keyboard message.
 */
typedef struct DataMessage {
  uint8_t destination = myAddr;
  uint8_t source = 0;
  uint8_t data = 0;
} DataMessage;

/// The last message that was received.
DataMessage lastMsg;

/// Whether a message was received (i.e., lastMsg has valid contents).
bool messageReceived;


void setup() {
  messageReceived = false;
  Wire.begin(myAddr);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(115200);           // start serial for output
  Serial.println(F("slave_receiver ready."));
}

/**
 * \brief When a message was received, create and send a response message.
 */
void loop() {
  if (messageReceived) {
    // Craft a response
    DataMessage response;
    response.destination = lastMsg.source >> 1; // guesswork: shift by 1. Observation: Keyboard that sent with 20 gets its response on 10.
    response.source = lastMsg.destination << 1; // guesswork: shift by 1. Observation: Central that received on 0x7F sends from 0xFE
    response.data = lastMsg.data;
    SendMessage(response);
    messageReceived = false;
  }
}

/**
 * \brief Send a given message over I2C.
 */
void SendMessage(const DataMessage& msg) {
  Wire.beginTransmission(msg.destination);
  Wire.write(msg.source);
  Wire.write(msg.data);
  Wire.endTransmission();
  Serial.println(F("Response sent."));
}

/// Obtian the de-masked sender address.
uint8_t extractSender(uint8_t source) {
  return (source & 0b00011110) >> 1;
}

/// Obtian the de-masked decoder output address.
uint8_t extractDecoderOut(uint8_t data) {
  uint8_t lowerBits = data & kDataLowerAddrMask;
  lowerBits >>= 1;

  uint8_t upperBits = data & kDataUpperAddrMask;
  upperBits >>= 2;

  uint8_t addr = upperBits | lowerBits;
  return addr;
}

/**
 * \brief Obtain the complete turnout address.
 *
 * The result is 0-based. Pressing a button for Turnout 1
 * on the first Keyboard is Address 0.
 */
uint8_t extractTurnoutAddr(const DataMessage& msg) {
  uint8_t addr = 0;
  addr |= extractSender(msg.source);
  addr <<= 4;
  addr |= extractDecoderOut(msg.data);
  return addr;
}

/// Get the direction a turnout should be switched to.
uint8_t extractDirection(uint8_t data) {
  return (data & kDataDirMask);
}

/// Whether power is to be switched on or off.
uint8_t extractPower(uint8_t data) {
  return (data & kDataPowerMask) >> 3;
}

void PrintMessage(const DataMessage& msg) {
  Serial.print('[');
  Serial.print(msg.destination, BIN);
  Serial.print(' ');
  Serial.print(msg.source, BIN);
  Serial.print(' ');
  Serial.print(msg.data, BIN);
  Serial.print(']');
  
  // Sender
  Serial.print(F(" Keyboard: "));
  Serial.print(extractSender(msg.source), DEC);

  Serial.print(F(" Decoder: "));
  Serial.print(extractDecoderOut(msg.data), DEC);
  
  Serial.print(F(" (Turnout Addr: "));
  Serial.print(extractTurnoutAddr(msg), DEC);

  Serial.print(F(") Direction: "));
  switch (extractDirection(msg.data)) {
    case kDataDirRed:
      Serial.print(F("RED  "));
      break;
    case kDataDirGreen:
      Serial.print(F("GREEN"));
      break;
    default:
      Serial.print(F("WTF"));
      break;
  }

  Serial.print(F(" Power: "));
  switch (extractDirection(msg.data)) {
    case 0:
      Serial.print(F("OFF"));
      break;
    case 1:
      Serial.print(F("ON "));
      break;
    default:
      Serial.print(F("WTF"));
      break;
  }

  Serial.println();
 
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
    PrintMessage(lastMsg);
    messageReceived = true;
  } else {
    Serial.println(F("Buffer full, lost message."));
  }
}
