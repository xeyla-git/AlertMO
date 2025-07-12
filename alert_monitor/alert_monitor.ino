#include <WiFi.h>
#include <WebServer.h>
#include <PINS_JC4827W543.h>
#include <TAMC_GT911.h>
#include "FreeSansBold12pt7b.h" // Optional: existing font used in your GIF project

// WiFi credentials
const char* ssid = "Alpha";
const char* password = "deblasio";

// Touchscreen setup
#define TOUCH_SDA 8
#define TOUCH_SCL 4
#define TOUCH_INT 3
#define TOUCH_RST 38
#define TOUCH_WIDTH 480
#define TOUCH_HEIGHT 272

TAMC_GT911 touchController(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

// Web server
WebServer server(80);
String alertMessage = "Awaiting alert...";

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Init display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
    while (true);
  }

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);  // Turn on backlight

  gfx->fillScreen(RGB565_BLACK);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextSize(1);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setCursor(10, 80);
  gfx->println(alertMessage);

  // Init touch
  touchController.begin();
  touchController.setRotation(ROTATION_INVERTED); // matches your working config

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Root page
  server.on("/", []() {
    String html = "<!DOCTYPE html><html><head><title>ESP Alert</title></head><body>";
    html += "<h1>Current Alert</h1><p>" + alertMessage + "</p></body></html>";
    server.send(200, "text/html", html);
  });

  // Receive alerts
  server.on("/alert", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      alertMessage = server.arg("plain");
      Serial.println("New Alert: " + alertMessage);
      drawAlert(alertMessage);
      server.send(200, "text/plain", "Alert received");
    } else {
      server.send(400, "text/plain", "Missing alert body");
    }
  });

  server.begin();
}

void loop() {
  server.handleClient();
  // Optional: add touch interaction (e.g., clear alert)
}

// Draw alert on screen
void drawAlert(String msg) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextSize(1);
  gfx->setTextColor(RGB565_RED);
  gfx->setCursor(10, 80);
  gfx->println("ALERT:");
  gfx->setCursor(10, 140);
  gfx->setTextColor(RGB565_WHITE);
  gfx->println(msg);
}
