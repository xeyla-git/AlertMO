#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SD.h>
#include <Arduino_GFX_Library.h>
#include <Arduino_ESP32RGBPanel.h>
#include <AnimatedGIF.h>
#include <TAMC_GT911.h>

// ==== WiFi Config ====
const char* ssid = "Alpha";
const char* password = "deblasio";

// ==== Alert State ====
String currentAlert = "";
bool alertActive = false;

// ==== Clear Button UI ====
#define CLEAR_BTN_X 340
#define CLEAR_BTN_Y 220
#define CLEAR_BTN_W 120
#define CLEAR_BTN_H 40

// ==== Touch Panel (JC4827W543C_I) ====
#define TOUCH_SDA     18
#define TOUCH_SCL      8
#define TOUCH_RST      4
#define TOUCH_INT      5
#define TOUCH_WIDTH  480
#define TOUCH_HEIGHT 272

TAMC_GT911 ts(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

// ==== SD ====
#define SD_CS 10
AnimatedGIF gif;
File gifFile;

// ==== Display (NV3041A RGB Panel) ====
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  39 /* DE */, 48 /* VSYNC */, 47 /* HSYNC */, 21 /* PCLK */,
  14 /* R0 */, 13 /* R1 */, 12 /* R2 */, 11 /* R3 */, 10 /* R4 */,
   9 /* G0 */, 46 /* G1 */,  3 /* G2 */,  8 /* G3 */, 16 /* G4 */, 15 /* G5 */,
   7 /* B0 */,  6 /* B1 */,  5 /* B2 */,  4 /* B3 */,  1 /* B4 */
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(480, 272, rgbpanel, 0 /* rotation */, true /* auto_flush */);

// ==== Web Server ====
WebServer server(80);

// ==== UI Functions ====
void drawClearButton() {
  gfx->fillRoundRect(CLEAR_BTN_X, CLEAR_BTN_Y, CLEAR_BTN_W, CLEAR_BTN_H, 8, BLUE);
  gfx->setTextColor(WHITE);
  gfx->setCursor(CLEAR_BTN_X + 20, CLEAR_BTN_Y + 12);
  gfx->setTextSize(2);
  gfx->print("Clear");
}

void showAlert(const String &msg) {
  gfx->fillScreen(RED);
  gfx->setTextColor(WHITE);
  gfx->setCursor(10, 50);
  gfx->setTextSize(3);
  gfx->print(msg);
  drawClearButton();
}

void clearAlert() {
  currentAlert = "";
  alertActive = false;
  gfx->fillScreen(BLACK);
}

// ==== Touch Check ====
void checkTouch() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    if (p.x >= CLEAR_BTN_X && p.x <= (CLEAR_BTN_X + CLEAR_BTN_W) &&
        p.y >= CLEAR_BTN_Y && p.y <= (CLEAR_BTN_Y + CLEAR_BTN_H)) {
      clearAlert();
    }
  }
}

// ==== GIF Drawing ====
void drawGIFFrame() {
  if (!alertActive && gif.open(gifFile, [](unsigned char *p, int len) {
    gifFile.read(p, len);
  })) {
    while (gif.playFrame(true, [](int x, int y, uint8_t *p, uint16_t t) {
      gfx->draw16bitRGBBitmap(x, y, (uint16_t*)p, gif.getCanvasWidth(), 1);
    }));
    gif.close();
    gifFile.seek(0);  // Loop
  }
}

// ==== Web Handlers ====
void handleAlertPost() {
  if (server.hasArg("plain")) {
    currentAlert = server.arg("plain");
    alertActive = true;
    showAlert(currentAlert);
    server.send(200, "text/plain", "Alert received.");
  } else {
    server.send(400, "text/plain", "Missing alert body.");
  }
}

void handleClearGet() {
  clearAlert();
  server.send(200, "text/plain", "Alert cleared.");
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);

  // Init Display
  gfx->begin();
  gfx->fillScreen(BLACK);

  // Init SD
  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
  } else {
    gifFile = SD.open("/images/bmo-adventure-time.gif");
    if (!gifFile) {
      Serial.println("GIF not found!");
    } else {
      gif.begin(LITTLE_ENDIAN_PIXELS);
    }
  }

  // Init Touch
  ts.begin();
  ts.setRotation(0);

  // Connect WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Start Web Server
  server.on("/alert", HTTP_POST, handleAlertPost);
  server.on("/clear", HTTP_GET, handleClearGet);
  server.begin();
}

// ==== Main Loop ====
void loop() {
  server.handleClient();
  if (alertActive) {
    checkTouch();
  } else if (gifFile) {
    drawGIFFrame();
  }
  delay(50);
}
