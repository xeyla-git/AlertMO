#include <SPI.h>
#include <SD.h>

#define SD_CS    10  // Likely CS pin
#define SD_SCK   12
#define SD_MISO  13
#define SD_MOSI  11

SPIClass spiSD(FSPI);  // FSPI for ESP32-S3

void setup() {
  Serial.begin(115200);
  delay(1000);

  spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, spiSD)) {
    Serial.println("SD init failed 😢");
  } else {
    Serial.println("SD init successful 🎉");
    File root = SD.open("/");
    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
      Serial.println(entry.name());
      entry.close();
    }
  }
}

void loop() {}
