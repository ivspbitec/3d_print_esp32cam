#include <WiFi.h>
#include <ArduinoOTA.h>
#include "SerialLogger.h"

const char* ssid = "TP-Link_094E";
const char* password = "68857301";

void setup() {
  SerialLog.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    SerialLog.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setHostname("esp32cam");
  ArduinoOTA.begin();
  SerialLog.println("OTA Ready");
  SerialLog.print("IP address: ");
  SerialLog.println(WiFi.localIP());
}

void loop() {
  ArduinoOTA.handle();
}
