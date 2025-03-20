#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "esp_system.h"
#include <ArduinoJson.h>
#include "SPIFFS.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 4   // GPIO 4 — мощный светодиод (Flash LED)
#endif

#define LCD_ONBOARD 0  // 1 - если есть встроенный LCD, 0 - если нет

struct DisplayData
{
    String ssid;
    String ip;
    int rssi;
    float temperature;
    String mqttCommand;
    bool isWifiConnected = false;
    bool isMqttConnected = false;
};

DisplayData globalData; // Глобальная переменная для хранения данных

#include "MQTTControl.h"
#include "CameraSetup.h"

#define LED_PIN 2
#define LED_COUNT 6
#include "LedControl.h"



#define DispSdaPin 15
#define DispSclPin 13
#include "LcdControl.h"

const char *apSSID = "ESP32_CAM";
const char *apPassword = "987654321S";

WebServer server(8080);
WebServer serverSettings(80);
Preferences preferences;

// #include <DNSServer.h>
// DNSServer dnsServer;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

int isWifiConnect = 0;
// int isScreenCapture=0;

void handleStream();
void handleSnapshot();
void handleLEDOn();
void handleLEDOff();

bool wifiConnect(String ssid, String password);
void wifiAP();

void handleRoot()
{
    serverSettings.send(200, "text/html", "<form method='POST' action='/save'><label>SSID: <input name='ssid'></label><br><label>Password: <input name='password'></label><br><input type='submit'></form>");
}

void handleSave()
{
    String ssid = serverSettings.arg("ssid");
    String password = serverSettings.arg("password");
    
    
    // Сохранение SSID и пароля в NVS
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    // Отключение точки доступа перед переключением на STA
    WiFi.softAPdisconnect(true);

    if (wifiConnect(ssid, password))
    {
        String redirectPage = "<html><head>";
        redirectPage += "<meta http-equiv='refresh' content='6;url=http://" + WiFi.localIP().toString() + "/stream'>";
        redirectPage += "</head><body>Saved and connected! The device will now restart.</body></html>";

        Serial.println(WiFi.localIP().toString());
        serverSettings.send(200, "text/html", redirectPage);
    }
    else
    {
        serverSettings.sendHeader("Location", "/", true); // Перенаправление на главную страницу
        serverSettings.send(302, "text/plain", "");
        wifiAP();
    }

    delay(1000); // Wait a bit before restarting
    ESP.restart();
}

void handleNotFound()
{
    serverSettings.sendHeader("Location", "/", true); // Перенаправление на главную страницу
    serverSettings.send(302, "text/plain", "");
}

bool wifiConnect(String ssid, String password)
{
    // Подключение к сохраненной сети Wi-Fi
    isWifiConnect = 0;
    
    // Настраиваем WiFi в режим STA с минимальным потреблением
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(true);  // Включаем режим энергосбережения
    WiFi.setAutoReconnect(true);  // Автоматическое переподключение
    
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Connecting to saved WiFi...");

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(2000);
        Serial.println("Connecting...");
        lcdPrint("Connecting...");
        if (millis() - startTime > 20000)
        { // Попытка подключения не более 20 секунд
            Serial.println("Connection timeout. Erasing WiFi credentials.");
            preferences.begin("wifi-config", false);
            preferences.clear(); // Очищаем настройки Wi-Fi
            preferences.end();
            break;
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Connected to saved WiFi");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        globalData.ssid = WiFi.SSID();
        globalData.ip = WiFi.localIP().toString();
        globalData.rssi = WiFi.RSSI();
        globalData.isWifiConnected = true;

        lcdPrint("Wifi Connected: %s", WiFi.localIP().toString());

        isWifiConnect = 1;
        return true;
    }
    return false;
}

void wifiAP()
{
    // WiFi.softAPdisconnect(true);
    // WiFi.disconnect();

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);

    // dnsServer.start(DNS_PORT, "*", apIP);

    IPAddress apIP = WiFi.softAPIP();
    Serial.print("Access Point IP address: ");
    Serial.println(apIP);
    lcdPrint("Access Point SSID: %s, Password: %s, IP: %s", apSSID, apPassword, apIP.toString().c_str());
}

void wifiInit()
{
    // Отключаем WiFi перед инициализацией
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    // Проверка сохраненных настроек Wi-Fi
    preferences.begin("wifi-config", true);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();

    if (ssid != "")
    {
        wifiConnect(ssid, password);
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        wifiAP();
    }
}

unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 30000; // проверка каждые 30 сек

/** Проверка подключения к wifi в процессе работы*/
void checkWiFiConnection()
{
    unsigned long now = millis();
    if (now - lastWifiCheck >= wifiCheckInterval)
    {

        lastWifiCheck = now;

        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi disconnected, attempting reconnect...");
            lcdPrint("WiFi lost, reconnecting...");

            preferences.begin("wifi-config", true);
            String ssid = preferences.getString("ssid", "");
            String password = preferences.getString("password", "");
            preferences.end();

            if (ssid != "")
            {
                wifiConnect(ssid, password);
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.println("WiFi reconnected successfully!");
                lcdPrint("WiFi reconnected: %s", WiFi.localIP().toString());
                globalData.isWifiConnected = true;
            }
            else
            {
                Serial.println("WiFi reconnection failed!");
                lcdPrint("WiFi reconnect failed!");
                globalData.isWifiConnected = false;
                delay(1000);  
                ESP.restart();                
            }
        }
    }
}

void handleReset()
{
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    preferences.begin("settings", false);
    preferences.clear();
    preferences.end();
}

// Определение версии из Git
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0
#define VERSION_BUILD __TIME__[6] // Используем секунды из времени компиляции как build number

#define VERSION_STRING String(VERSION_MAJOR) + "." + String(VERSION_MINOR) + "." + String(VERSION_PATCH) + "." + String(VERSION_BUILD)

void handleSettings()
{
    preferences.begin("settings", true);
    String led_r = preferences.getString("led_r", "255");
    String led_g = preferences.getString("led_g", "255");
    String led_b = preferences.getString("led_b", "255");
    int brightness = preferences.getInt("brightness", 0);

    int resolution = preferences.getInt("resolution", FRAMESIZE_SVGA);
    bool flip_vertical = preferences.getBool("flip_vertical", false);
    int camera_quality = preferences.getInt("camera_quality", 12);

    String mqtt_server = preferences.getString("mqtt_server", "");
    int mqtt_port = preferences.getInt("mqtt_port", 1883);
    String mqtt_user = preferences.getString("mqtt_user", "");
    String mqtt_password = preferences.getString("mqtt_password", "");
    preferences.end();

    // Читаем шаблон из SPIFFS
    File file = SPIFFS.open("/templates/settings.html", "r");
    String html = file.readString();
    file.close();

    // Заменяем плейсхолдеры на реальные значения
    html.replace("%LED_R%", led_r);
    html.replace("%LED_G%", led_g);
    html.replace("%LED_B%", led_b);
    html.replace("%BRIGHTNESS%", String(brightness));
    html.replace("%FLIP_VERTICAL%", flip_vertical ? "checked" : "");
    html.replace("%VERSION%", VERSION_STRING);
    
    // Заменяем значения для resolution
    const int resolutions[] = {FRAMESIZE_QQVGA, FRAMESIZE_QCIF, FRAMESIZE_HQVGA, FRAMESIZE_QVGA, 
                             FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA, FRAMESIZE_HD, 
                             FRAMESIZE_SXGA, FRAMESIZE_UXGA};
    const String resNames[] = {"QQVGA", "QCIF", "HQVGA", "QVGA", "VGA", "SVGA", "XGA", "HD", "SXGA", "UXGA"};
    
    for (int i = 0; i < 10; i++) {
        html.replace("%" + resNames[i] + "_SELECTED%", resolution == resolutions[i] ? "selected" : "");
        html.replace("%FRAMESIZE_" + resNames[i] + "%", String(resolutions[i]));
    }

    // Заменяем значения для quality
    const int qualities[] = {5, 7, 10, 12, 15, 17, 20, 25, 30, 40};
    for (int q : qualities) {
        html.replace("%QUALITY_" + String(q) + "%", String(q));
        html.replace("%QUALITY_" + String(q) + "_SELECTED%", camera_quality == q ? "selected" : "");
    }

    html.replace("%MQTT_SERVER%", mqtt_server);
    html.replace("%MQTT_PORT%", String(mqtt_port));
    html.replace("%MQTT_USER%", mqtt_user);
    html.replace("%MQTT_PASSWORD%", mqtt_password);

    server.send(200, "text/html", html);
}

void handleSettingsSave()
{
    String led_r = server.arg("led_r");
    String led_g = server.arg("led_g");
    String led_b = server.arg("led_b");
    int brightness = server.arg("brightness").toInt();

    String mqtt_server = server.arg("mqtt_server");
    String mqtt_port = server.arg("mqtt_port");
    String mqtt_user = server.arg("mqtt_user");
    String mqtt_password = server.arg("mqtt_password");

    preferences.begin("settings", false);
    preferences.putString("led_r", led_r);
    preferences.putString("led_g", led_g);
    preferences.putString("led_b", led_b);
    preferences.putInt("brightness", brightness);

    int resolution = server.arg("resolution").toInt();
    bool flip_vertical = server.arg("flip_vertical") == "on";
    int camera_quality = server.arg("camera_quality").toInt();

    preferences.putInt("camera_quality", camera_quality);
    preferences.putInt("resolution", resolution);
    preferences.putBool("flip_vertical", flip_vertical);

    mqtt_server.trim();
    mqtt_user.trim();
    mqtt_password.trim();

    preferences.putString("mqtt_server", mqtt_server);
    preferences.putInt("mqtt_port", mqtt_port.toInt());
    preferences.putString("mqtt_user", mqtt_user);
    preferences.putString("mqtt_password", mqtt_password);

    preferences.end();

    server.sendHeader("Location", "/settings", true);
    server.send(303); // Код состояния 303 See Other

    delay(500);

    ESP.restart();
}


unsigned long lastTemperatureAttempt = 0;
const unsigned long temperatureInterval = 10000; // 2 секунды
void temperatureLoop()
{
    unsigned long now = millis();
    if (now - lastTemperatureAttempt >= temperatureInterval)
    {
        lastTemperatureAttempt = now;
        float temperature = (float)temperatureRead();
        globalData.temperature = temperature;
        mqttTemperature(String(temperature));
    }
}


int lastResolution = -1;
int lastQuality = -1;
const unsigned long cameraStateInterval = 1000; // Проверка каждые 10 секунд
unsigned long lastCameraStateCheck = 0;

void cameraStateLoop()
{
    unsigned long now = millis();
    if (now - lastCameraStateCheck >= cameraStateInterval)
    {
        lastCameraStateCheck = now;
        preferences.begin("settings", true);
        int currentResolution = preferences.getInt("resolution", FRAMESIZE_SVGA);
        int currentQuality = preferences.getInt("camera_quality", 12);
        preferences.end();

        if (currentResolution != lastResolution || currentQuality != lastQuality)
        {
            lastResolution = currentResolution;
            lastQuality = currentQuality;
            mqttCameraState(currentResolution, currentQuality);
        }
    }
}


unsigned long lastBuiltinLedCheck = 0;
const unsigned long builtinLedCheckInterval = 1000; // Проверка каждую секунду
bool lastBuiltinLedState = LOW;
void builtinLedStateLoop()
{
    unsigned long now = millis();
    if (now - lastBuiltinLedCheck >= builtinLedCheckInterval)
    {
        lastBuiltinLedCheck = now;
        bool currentLedState = digitalRead(LED_BUILTIN);
        if (currentLedState != lastBuiltinLedState)
        {
            lastBuiltinLedState = currentLedState;
            mqttBuiltinLedState(currentLedState ? "ON" : "OFF");
        }
    }
}



void setup()
{
    Serial.begin(115200);
    delay(1000);  // Даём время на инициализацию Serial
    Serial.println("Starting...");

    // Отключаем неиспользуемые функции для экономии памяти
    btStop();  // Отключаем Bluetooth
 

    pinMode(LED_BUILTIN, OUTPUT);    
    digitalWrite(LED_BUILTIN, LOW);  

    // Экран
    #if LCD_ONBOARD
    lcdInit();
    delay(100);
    #endif

    // Лента
    strip.begin();
    strip.show();
    delay(100);

    if(!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        delay(1000);
        ESP.restart();
        return;
    }

    wifiInit();

    if (isWifiConnect != 1)
    {
        serverSettings.on("/", handleRoot);
        serverSettings.on("/save", HTTP_POST, handleSave);
        serverSettings.onNotFound(handleNotFound);
        serverSettings.begin();
    }
    else
    {
        lcdPrint("Camera Init");
        delay(100);
 
        cameraInit();
        delay(100);

        server.on("/stream", HTTP_GET, handleStream);
        server.on("/snapshot", HTTP_GET, handleSnapshot);
        server.on("/reset", HTTP_GET, handleReset);
        server.on("/settings", HTTP_GET, handleSettings);
        server.on("/save", HTTP_POST, handleSettingsSave);

        server.on("/", HTTP_GET, []()
                  {
                if (server.arg("action") == "stream") {
                    handleStream();
                }
                else  if (server.arg("action") == "snapshot") {
                    handleSnapshot();
                } });

        server.on("/led_on", handleLEDOn);
        server.on("/led_off", handleLEDOff);

        server.begin();

        lcdPrint("MQTT Init");
        Serial.println("MQTT Init");
        delay(100);
        mqttInit();
    }
}

void commonLoop()
{
    server.handleClient();
    mqttLoop();
    #if LCD_ONBOARD
    if (getLcdState())
    {
        updateDisplay();
    }
    #endif
    
    temperatureLoop();
    builtinLedStateLoop();
    cameraStateLoop();

    checkWiFiConnection();
}

void loop()
{

    // dnsServer.processNextRequest();

    if (isWifiConnect != 1)
    {
        serverSettings.handleClient();
    }
    else
    {
        commonLoop();
    }
}

void handleLEDOn()
{
    LedOn();
    server.send(200, "text/html", "LED ON");
}

void handleLEDOff()
{
    LedOff();
    server.send(200, "text/html", "LED OFF");
}

// Функция для настройки сервера вещания с камеры
void startCameraServer()
{
    server.on("/stream", HTTP_GET, []()
              {
            //server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request){
            WiFiClient client = server.client();
            camera_fb_t* fb = NULL;
            Serial.println("Stream Start");
            if (!client.connected()) {
                return;
            }
            String response = "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
            client.write(response.c_str(), response.length());
            int iterationCount = 0;
            while (client.connected()) {
                fb = esp_camera_fb_get();
                if (!fb) {
                    Serial.println("Camera capture failed");
                    break;
                }

                response = "--frame\r\n";
                response += "Content-Type: image/jpeg\r\n\r\n";
                client.write(response.c_str(), response.length());
                client.write(fb->buf, fb->len);
                client.write("\r\n", 2);

                esp_camera_fb_return(fb);

                iterationCount++;
                if (iterationCount % 10 == 0) {
                    server.handleClient();
                    iterationCount = 0;
                }

            }
            Serial.println("Stream End"); });
}

void handleStream()
{
    WiFiClient client = server.client();
    camera_fb_t *fb = NULL;
    Serial.println("Stream Start");
    if (!client.connected())
    {
        return;
    }
    String response = "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    client.write(response.c_str(), response.length());

    int counter = 0;
    while (client.connected())
    {
        fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("Camera capture failed");
            break;
        }

        response = "--frame\r\n";
        response += "Content-Type: image/jpeg\r\n\r\n";
        client.write(response.c_str(), response.length());
        client.write(fb->buf, fb->len);
        client.write("\r\n", 2);

        esp_camera_fb_return(fb);

        // Обработка запросов сервера каждые 10 итераций
        if (counter++ % 3 == 0)
        {
            commonLoop();
        }
    }
    Serial.println("Stream End");
}

void handleSnapshot()
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        server.send(500, "text/plain", "Camera capture failed");
        return;
    }

    WiFiClient client = server.client();
    if (!client.connected())
    {
        esp_camera_fb_return(fb);
        return;
    }

    String response = "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\n Content-Length: " + String(fb->len) + "\r\n\r\n";

    client.write(response.c_str(), response.length());
    client.write(fb->buf, fb->len);
    client.write("\r\n", 2);

    esp_camera_fb_return(fb);
}


