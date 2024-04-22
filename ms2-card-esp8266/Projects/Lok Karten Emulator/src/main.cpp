#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "Wire.h"
#include "I2C_eeprom.h"
#include "BR_xx.h"

#define SPI_MISO    12
#define SPI_MOSI    13
#define SPI_CLK     14
#define SPI_CS      15

#define I2C_Select   2
#define I2C_SDA      4
#define I2C_SCL      5
#define Card_Change 16

#define LOCO_SIZE   272

I2C_eeprom ee(0x50, I2C_DEVICESIZE_24LC64);
uint8_t ee_buffer[LOCO_SIZE];
uint8_t loco_id = 10;
int sd_card;
File root;

void copy_loco_data(){
  uint16_t i;
  for (i = 0; i < LOCO_SIZE; i++) {
    ee_buffer[i] = pgm_read_byte_far(&BR86_bin[i]);
  }
}

int write_loco2fram() {
  int ret;

  /* access I2C memory by ESP8266 */
  digitalWrite(I2C_Select, 1);

  ret = ee.isConnected();
  if (!ret) {
    Serial.printf("Error: 0x%20X Can't find FRAM", ret);
    Serial.println();
  }

  ret = ee.writeBlock(0, ee_buffer, LOCO_SIZE);
  if (ret) {
    Serial.printf("Error: 0x%20X Can't write FRAM", ret);
    Serial.println();   
  }
  /* release I2C memory - back to MS2 */
  digitalWrite(I2C_Select, 0);
  return ret;
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void setup() {
  delay(3000);
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.println("Serial EEPROM version:");
  Serial.println(I2C_EEPROM_VERSION);

  
  pinMode(Card_Change, OUTPUT);
  pinMode(I2C_Select, OUTPUT);
  digitalWrite(Card_Change, 0);
  copy_loco_data();

  Serial.print("Initialising SD Card ... ");
  if (!SD.begin(SPI_CS)) {
    Serial.println("initialization failed - no SD Card inserted ?");
    sd_card = 0;
  } else {
    Serial.println("success !");
    sd_card = 1;
  }

  if (sd_card) {
    root = SD.open("/");
    printDirectory(root, 0);
  }

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  ee.begin();
  write_loco2fram();
}


void loop() {
  Serial.println("Cycle");
  delay(2000);
  loco_id = loco_id +1;
  if (loco_id > 70)
     loco_id = 10;
  ee_buffer[246]++;
  if (ee_buffer[246] > 0x39) {
    ee_buffer[246]=0x30;
  }
  ee_buffer[5]=loco_id;
  ee_buffer[11]=loco_id;
  Serial.printf("Loco ID: %d\n", loco_id);
  digitalWrite(Card_Change, 0);
  write_loco2fram();
  Serial.println("Card Change");
  delay(500);
  digitalWrite(Card_Change, 1);
}

