#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>

#include "I2C_eeprom.h"
#include "BR_xx.h"
#include "loco_sdcard.h"
#include "webserver.h"

#define SPI_MISO    12
#define SPI_MOSI    13
#define SPI_CLK     14
#define SPI_CS      15

#define I2C_Select   2
#define I2C_SDA      4
#define I2C_SCL      5
#define Card_Change 16

#define LOCO_BIN_SIZE      272
#define LOCO_BIN_SIZE_MAX 8192

const char *ssid = "LokkartenEMU";
const char *password = "bingbing";
IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);
ESP8266WebServer server(80);

const char *ESPHostname = "LokEmu";
const byte DNS_PORT = 53;
DNSServer dnsServer;

I2C_eeprom ee(0x50, I2C_DEVICESIZE_24LC64);
uint8_t ee_buffer[LOCO_BIN_SIZE_MAX];
uint8_t loco_id = 10;
int sd_card, cycle_count, sd2ms2;
File root;


/* Test with simple BIN File of BR86 */
void copy_loco_data(){
  uint16_t i;
  for (i = 0; i < LOCO_BIN_SIZE; i++) {
    ee_buffer[i] = pgm_read_byte_far(&BR86_bin[i]);
  }
}

int write_loco2fram(unsigned int size) {
  int ret; /* TODO*/

  if (size > sizeof(ee_buffer)) {
    Serial.println("BIN to large for FRAM");
    return -1;
  } else {
    /* access I2C memory by ESP8266 */
    digitalWrite(I2C_Select, 1);

    ret = ee.isConnected();
    if (!ret) {
      Serial.printf("Error: 0x%20X Can't find FRAM", ret);
      Serial.println();
      digitalWrite(I2C_Select, 0);
      return ret;
    }

    ret = ee.writeBlock(0, ee_buffer, size);
    if (ret) {
      Serial.printf("Error: 0x%20X Can't write FRAM", ret);
      Serial.println();
      digitalWrite(I2C_Select, 0);
      return ret;   
    }
  }
  /* release I2C memory - back to MS2 */
  digitalWrite(I2C_Select, 0);
  return 0;
}

void setup() {
  /* make emu passive first */
  pinMode(Card_Change, OUTPUT);
  pinMode(I2C_Select, OUTPUT);
  digitalWrite(Card_Change, 0);
  digitalWrite(I2C_Select, 0);
  delay(2000);

  Serial.begin(115200);

  Serial.println("Serial EEPROM version:");
  Serial.println(I2C_EEPROM_VERSION);

  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(ssid,password) ? "Ready" : "Failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
  delay(100);
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", local_IP);

  server.on("/", handle_OnConnect);
  server.on("/sd2ms2_go", handle_OnConnect_sd2ms2_go);
  server.on("/sd2ms2_stop", handle_OnConnect_sd2ms2_stop);
  server.onNotFound(handle_NotFound);
  server.begin();
  

  Serial.print("Initialising SD Card ... ");
  if (!SD.begin(SPI_CS)) {
    Serial.println("initialization failed - no SD Card inserted ?");
    sd_card = 0;
  } else {
    Serial.println("success :-)");
    sd_card = 1;
  }

  if (sd_card) {
    root = SD.open("/LocoCards");
    if (root) {
      printDirectory(root, 0);
    } else {
      Serial.println("no directorty /LocoCards !");
    }
  }

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  ee.begin();
  //copy_loco_data();
  //write_loco2fram(LOCO_BIN_SIZE);
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest(); //DNS
  Serial.printf("Cycle %d\n", cycle_count++);
  delay(500);
}

