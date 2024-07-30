#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "esp_system.h"
#include <ArduinoJson.h>

struct DisplayData {
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

const char* apSSID = "ESP_32CM";
const char* apPassword = "987654321S";

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

void handleRoot() {
    serverSettings.send(200, "text/html", "<form method='POST' action='/save'><label>SSID: <input name='ssid'></label><br><label>Password: <input name='password'></label><br><input type='submit'></form>");
}

void handleSave() {
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

void handleNotFound() {
    serverSettings.sendHeader("Location", "/", true); // Перенаправление на главную страницу
    serverSettings.send(302, "text/plain", "");
}

bool wifiConnect(String ssid, String password) {
    // Подключение к сохраненной сети Wi-Fi
    isWifiConnect = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Connecting to saved WiFi...");

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED)


    {
        delay(1000);
        Serial.println("Connecting...");
        lcdPrint("Connecting...");
        if (millis() - startTime > 10000)
        { // Попытка подключения не более 10 секунд
            Serial.println("Connection timeout. Erasing WiFi credentials.");
            preferences.begin("wifi-config", false);
            preferences.clear(); // Очищаем настройки Wi-Fi
            preferences.end();

            break;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
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

void wifiAP() {
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

void wifiInit() {

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

void handleReset() {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    preferences.begin("settings", false);
    preferences.clear();
    preferences.end();
}

void handleSettings() {

    preferences.begin("settings", true);
    String led_r = preferences.getString("led_r", "255");
    String led_g = preferences.getString("led_g", "255");
    String led_b = preferences.getString("led_b", "255");
    String brightness = preferences.getString("brightness", "255");

    String camera_width = preferences.getString("camera_width", "");
    String camera_height = preferences.getString("camera_height", "");

    String mqtt_server = preferences.getString("mqtt_server", "");
    int mqtt_port = preferences.getInt("mqtt_port", 1883);
    String mqtt_user = preferences.getString("mqtt_user", "");
    String mqtt_password = preferences.getString("mqtt_password", "");
    preferences.end();
    // 192.168.0.104
    // esp32cam
    // esP365
    server.send(200, "text/html", "<form method='POST' action='/save'>"
        "<h2>Led</h2><br>"
        "<label>R: <input name='led_r' value='" +
        led_r + "'></label><br>"
        "<label>G: <input name='led_g' value='" +
        led_g + "'></label><br>"
        "<label>B: <input name='led_b' value='" +
        led_b + "'></label><br><br>"

        "<label>B: <input name='brightness' value='" +
        brightness + "'></label><br><br>"

        "<h2>Camera</h2><br>"
        "<label>Width: <input name='led_g' value='" +
        camera_width + "'></label><br>"
        "<label>Height: <input name='led_g' value='" +
        camera_height + "'></label><br>"

        "<h2>MQTT</h2><br>"
        "<label>Server: <input name='mqtt_server' value='" +
        mqtt_server + "'></label><br>"
        "<label>Port: <input name='mqtt_port' value='" +
        mqtt_port + "'></label><br>"
        "<label>User: <input name='mqtt_user' value='" +
        mqtt_user + "'></label><br>"
        "<label>Password: <input name='mqtt_password' value='" +
        mqtt_password + "'></label><br>"

        "<br>"
        "<input type='submit'></form>");
}

void handleSettingsSave() {
    String led_r = server.arg("led_r");
    String led_g = server.arg("led_g");
    String led_b = server.arg("led_b");
    String brightness = server.arg("brightness");

    String camera_height = server.arg("camera_height");
    String camera_width = server.arg("camera_width");

    String mqtt_server = server.arg("mqtt_server");
    String mqtt_port = server.arg("mqtt_port");
    String mqtt_user = server.arg("mqtt_user");
    String mqtt_password = server.arg("mqtt_password");

    preferences.begin("settings", false);
    preferences.putString("led_r", led_r);
    preferences.putString("led_g", led_g);
    preferences.putString("led_b", led_b);
    preferences.putString("brightness", brightness);
    preferences.putString("camera_width", camera_width);
    preferences.putString("camera_height", camera_height);

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

    ESP.restart();
}

void setup() {
    Serial.begin(115200);

    // Экран
    lcdInit();

    // Лента
    strip.begin();
    strip.show();

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

        cameraInit();

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
                mqttInit();
    }
}

void loop() {

    // dnsServer.processNextRequest();

    if (isWifiConnect != 1)
    {
        serverSettings.handleClient();
    }
    else
    {
        server.handleClient();
        mqttLoop();
        if (getLcdState())
        {
            updateDisplay();
        }
    }
}

void handleLEDOn() {
    LedOn();
    server.send(200, "text/html", "LED ON");
}

void handleLEDOff() {
    LedOff();
    server.send(200, "text/html", "LED OFF");
}

// Функция для настройки сервера вещания с камеры
void startCameraServer() {
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

void handleStream() {
    WiFiClient client = server.client();
    camera_fb_t* fb = NULL;
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
        if (counter++ % 10 == 0)
        {
            server.handleClient();
        }
    }
    Serial.println("Stream End");
}

void handleSnapshot() {
    camera_fb_t* fb = esp_camera_fb_get();
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