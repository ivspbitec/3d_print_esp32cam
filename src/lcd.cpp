#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>


const char* apSSID = "ESP32-CAM";
const char* apPassword = "132465";

WebServer server(80);
Preferences preferences;
 
#define DispSdaPin 14
#define DispSclPin 13

 

// библиотеки для работы с OLED экраном Arduino IDE
#include "Wire.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"


Adafruit_SSD1306 display(128, 32, &Wire, -1); // указываем размер экрана в пикселях


 
void handleRoot() {
    server.send(200, "text/html", "<form method='POST' action='/save'><label>SSID: <input name='ssid'></label><br><label>Password: <input name='password'></label><br><input type='submit'></form>");
}

void handleSave() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    // Сохранение SSID и пароля в NVS
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    // Отключение точки доступа перед переключением на STA
    WiFi.softAPdisconnect(true);

    // Подключение к Wi-Fi
    WiFi.begin(ssid.c_str(), password.c_str());
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting...");
    }

    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.send(200, "text/html", "Saved! The device will now restart.");
    
    delay(1000); // Wait a bit before restarting
    ESP.restart();
}

void handleNotFound() {
    server.sendHeader("Location", "/", true); // Перенаправление на главную страницу
    server.send(302, "text/plain", "");
}

void lcdPrint(const char *format, ...) {
    char buffer[256]; // Буфер для хранения строки
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    display.clearDisplay();
    display.setTextSize(1, 1); // Указываем размер шрифта
    display.setTextColor(SSD1306_WHITE); // Указываем цвет надписи
    display.setCursor(0, 0);
    display.println(buffer);
    display.display();
}


void lcdInit(){
  Wire.begin(DispSdaPin,DispSclPin);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address SclPin for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
   Serial.println("LCD Init"); 
}

void setup() {
    Serial.begin(115200);
    lcdInit();

    // Проверка сохраненных настроек Wi-Fi
    preferences.begin("wifi-config", true);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();

    if (ssid != "") {
        // Подключение к сохраненной сети Wi-Fi
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.println("Connecting to saved WiFi...");
        lcdPrint("Connecting to saved WiFi...");
         
        unsigned long startTime = millis();

        while (WiFi.status() != WL_CONNECTED) { 
            delay(1000); 
            Serial.println("Connecting...");
            if (millis() - startTime > 4000) { // Попытка подключения не более 10 секунд
                Serial.println("Connection timeout. Erasing WiFi credentials.");
                lcdPrint("Connection timeout. Erasing WiFi credentials.");
                preferences.begin("wifi-config", false);
                preferences.clear(); // Очищаем настройки Wi-Fi
                preferences.end();
                break;
            }
        } 

        if (WiFi.status() == WL_CONNECTED) {          
            Serial.println("Connected to saved WiFi");
            Serial.print("IP address: ");
            lcdPrint("Wifi Connected. SSID: %s, IP: %s", WiFi.BSSIDstr(), WiFi.localIP().toString().c_str());
            Serial.println(WiFi.localIP());
        }
    } 
    
    if (WiFi.status() != WL_CONNECTED){
        // Настройка точки доступа  
        WiFi.mode(WIFI_AP);
        WiFi.softAP(apSSID, apPassword);
        IPAddress apIP = WiFi.softAPIP();
        Serial.print("Access Point IP address: ");
        Serial.println(apIP);        
        lcdPrint("Access Point SSID: %s, Password: %s, IP: %s", apSSID, apPassword, apIP.toString().c_str());
    }

    // Настройка веб-сервера
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    //server.onNotFound(handleNotFound); 
    server.begin();

     
 
}

void loop() {
  //  server.handleClient();
}
 