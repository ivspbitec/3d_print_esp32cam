/*
  OTA обновление:

  upload_protocol = espota
  build_flags = -DOTA_HOSTNAME="camera1"

  Заливка по Wi-Fi:
  pio run -t upload --upload-port camera1.local
*/

// #include <DeviceConfig.h> // Путь к файлу DeviceConfig.h
#include "SerialLogger.h"
SerialLogger SerialLog; // Объявление переменной SerialLog

#include <WiFi.h>        // Библиотека для работы с WiFi
#include <WebServer.h>   // Веб-сервер
#include <Preferences.h> // Для работы с Flash
#include "esp_system.h"  // Для работы с ESP32
#include <ArduinoJson.h> // Библиотека для работы с JSON
#include <ArduinoOTA.h>  //Обновление по WiFi

// Add this block to ensure OTA_HOSTNAME is defined
#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "esp32cam"
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 4 // GPIO 4 — мощный светодиод (Flash LED)
#endif

bool useMQTTBuiltinLed = false;

/** Режим прошивки */
volatile bool otaRunning = false;

struct DisplayData
{
    String ssid;
    String ip;
    int rssi;
    float temperature;
    String mqttCommand;
    bool isWifiConnected = false;
    bool isMqttConnected = false;
    String otaHost; // Добавлено поле для OTA host
};

DisplayData globalData; // Глобальная переменная для хранения данных

#include "MQTTControl.h"
#include "CameraSetup.h"

#if (LED_ONBOARD)
#include "LedControl.h"
#endif

#if (LCD_ONBOARD)
#define DispSdaPin 15
#define DispSclPin 13
#include "LcdControl.h"
#endif

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

// Определение версии из Git
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0
#define VERSION_BUILD __TIME__[6] // Используем секунды из времени компиляции как build number

#define VERSION_STRING String(VERSION_MAJOR) + "." + String(VERSION_MINOR) + "." + String(VERSION_PATCH) + "." + String(VERSION_BUILD)

#include "static_files.h"

void handleStream();
void handleSnapshot();
void handleLEDOn();
void handleLEDOff();
void handleSerialLog();

bool wifiConnectMulti();
void wifiAP();

void handleRoot()
{
    String html(AP_SETTINGS_HTML);
    html.replace("%VERSION%", VERSION_STRING); 

    // Отправляем клиенту
    serverSettings.send(200, "text/html", html);
}

void handleSave()
{
    String ssid1 = serverSettings.arg("ssid1");
    String password1 = serverSettings.arg("password1");
    String ssid2 = serverSettings.arg("ssid2");
    String password2 = serverSettings.arg("password2");
    String ssid3 = serverSettings.arg("ssid3");
    String password3 = serverSettings.arg("password3");

    preferences.begin("wifi-config", false);
    preferences.putString("ssid1", ssid1);
    preferences.putString("password1", password1);
    preferences.putString("ssid2", ssid2);
    preferences.putString("password2", password2);
    preferences.putString("ssid3", ssid3);
    preferences.putString("password3", password3);
    preferences.end();

    WiFi.softAPdisconnect(true);

    if (wifiConnectMulti())
    {
        String redirectPage = "<html><head>";
        redirectPage += "<meta http-equiv='refresh' content='6;url=http://" + WiFi.localIP().toString() + "/stream'>";
        redirectPage += "</head><body>Saved and connected! The device will now restart.</body></html>";
        SerialLog.println(WiFi.localIP().toString());
        serverSettings.send(200, "text/html", redirectPage);
    }
    else
    {
        serverSettings.sendHeader("Location", "/", true);
        serverSettings.send(302, "text/plain", "");
        wifiAP();
    }

    delay(1000);
    ESP.restart();
}

void handleNotFound()
{
    serverSettings.sendHeader("Location", "/", true); // Перенаправление на главную страницу
    serverSettings.send(302, "text/plain", "");
}

bool wifiConnectMulti()
{
    isWifiConnect = 0;
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(true);
    WiFi.setAutoReconnect(true);

    preferences.begin("wifi-config", true);
    String ssid[3], password[3];
    ssid[0] = preferences.getString("ssid1", "");
    password[0] = preferences.getString("password1", "");
    ssid[1] = preferences.getString("ssid2", "");
    password[1] = preferences.getString("password2", "");
    ssid[2] = preferences.getString("ssid3", "");
    password[2] = preferences.getString("password3", "");
    int last_wifi_index = preferences.getInt("last_wifi_index", 0);
    preferences.end();

    // Перебираем сначала   с last_wifi_index, затем остальные по кругу 223
    for (int offset = 0; offset < 3; offset++)
    {
        int i = (last_wifi_index + offset) % 3;
        if (ssid[i].length() == 0)
            continue;
        WiFi.begin(ssid[i].c_str(), password[i].c_str());
        SerialLog.printf("Connecting to WiFi %d: %s\n", i + 1, ssid[i].c_str());
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED)
        {
            SerialLog.println("Connecting...");
            lcdPrint("Connecting...");
            if (millis() - startTime > 6000)
            {
                SerialLog.println("Connection timeout.");
                break;
            }
            delay(1000);
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            SerialLog.println("Connected to WiFi");
            SerialLog.print("IP address: ");
            SerialLog.println(WiFi.localIP());
            globalData.ssid = WiFi.SSID();
            globalData.ip = WiFi.localIP().toString();
            globalData.rssi = WiFi.RSSI();
            globalData.isWifiConnected = true;
            lcdPrint("Wifi Connected: %s", WiFi.localIP().toString());
            preferences.begin("wifi-config", false);
            preferences.putBool("ap", false);
            preferences.putInt("last_wifi_index", i); // Сохраняем индекс успешно подключенной сети
            preferences.end();
            isWifiConnect = 1;
            return true;
        }
    }
    // Если не удалось подключиться ни к одной сети, устанавливаем флаг ap
    preferences.begin("wifi-config", false);
    preferences.putBool("ap", true);
    preferences.end();
    return false;
}

unsigned long WifiAPStartTime = 0;                 // Время начала работы в режиме AP
const unsigned long WifiAPTimeout = 5 * 60 * 1000; // Таймаут 5 минут

void wifiAP()
{
    // WiFi.softAPdisconnect(true);
    // WiFi.disconnect();

    // WiFi.softAPdisconnect(true);
    // WiFi.disconnect();

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);

    // dnsServer.start(DNS_PORT, "*", apIP);

    IPAddress apIP = WiFi.softAPIP();
    SerialLog.print("Access Point IP address: ");
    SerialLog.println(apIP);
    lcdPrint("Access Point SSID: %s, Password: %s, IP: %s", apSSID, apPassword, apIP.toString().c_str());
}

void wifiInit()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    preferences.begin("wifi-config", true);
    String ssid1 = preferences.getString("ssid1", "");
    String password1 = preferences.getString("password1", "");
    String ssid2 = preferences.getString("ssid2", "");
    String password2 = preferences.getString("password2", "");
    String ssid3 = preferences.getString("ssid3", "");
    String password3 = preferences.getString("password3", "");
    bool apMode = preferences.getBool("ap", false);
    preferences.end();

    if (!apMode && (ssid1 != "" || ssid2 != "" || ssid3 != ""))
    {
        if (wifiConnectMulti())
            return;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        preferences.begin("wifi-config", true);
        preferences.putBool("ap", false); 
        preferences.end();
        wifiAP();
    }
}

unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 20000; // проверка каждые

/** Проверка подключения к wifi в процессе работы*/
void checkWiFiConnection()
{
    unsigned long now = millis();
    if (now - lastWifiCheck >= wifiCheckInterval)
    {

        lastWifiCheck = now;

        if (WiFi.status() != WL_CONNECTED)
        {
            SerialLog.println("WiFi disconnected, attempting reconnect...");
            lcdPrint("WiFi lost, reconnecting...");

            if (wifiConnectMulti())
            {
                SerialLog.println("WiFi reconnected successfully!");
                lcdPrint("WiFi reconnected: %s", WiFi.localIP().toString());
                globalData.isWifiConnected = true;
            }
            else
            {
                SerialLog.println("WiFi reconnection failed!");
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
    useMQTTBuiltinLed = preferences.getBool("use_builtin_led", false);
    preferences.end();

    String html(SETTINGS_HTML);  

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

    for (int i = 0; i < 10; i++)
    {
        html.replace("%" + resNames[i] + "_SELECTED%", resolution == resolutions[i] ? "selected" : "");
        html.replace("%FRAMESIZE_" + resNames[i] + "%", String(resolutions[i]));
    }

    // Заменяем значения для quality
    const int qualities[] = {5, 7, 10, 12, 15, 17, 20, 25, 30, 40};
    for (int q : qualities)
    {
        html.replace("%QUALITY_" + String(q) + "%", String(q));
        html.replace("%QUALITY_" + String(q) + "_SELECTED%", camera_quality == q ? "selected" : "");
    }

    html.replace("%MQTT_SERVER%", mqtt_server);
    html.replace("%MQTT_PORT%", String(mqtt_port));
    html.replace("%MQTT_USER%", mqtt_user);
    html.replace("%MQTT_PASSWORD%", mqtt_password);
    html.replace("%USE_BUILDIN_LED%", useMQTTBuiltinLed ? "checked" : "");

    preferences.begin("wifi-config", true);
    String ssid1 = preferences.getString("ssid1", "");
    String password1 = preferences.getString("password1", "");
    String ssid2 = preferences.getString("ssid2", "");
    String password2 = preferences.getString("password2", "");
    String ssid3 = preferences.getString("ssid3", "");
    String password3 = preferences.getString("password3", "");
    preferences.end();

    html.replace("%WIFI_SSID1%", ssid1);
    html.replace("%WIFI_PASSWORD1%", password1);
    html.replace("%WIFI_SSID2%", ssid2);
    html.replace("%WIFI_PASSWORD2%", password2);
    html.replace("%WIFI_SSID3%", ssid3);
    html.replace("%WIFI_PASSWORD3%", password3);

    // --- LCD info tab placeholders ---
    html.replace("%LCD_WIFI%", globalData.isWifiConnected ? "Подключено" : "Нет");
    html.replace("%LCD_MQTT%", globalData.isMqttConnected ? "Подключено" : "Нет");
    html.replace("%LCD_SSID%", globalData.ssid);
    html.replace("%LCD_IP%", globalData.ip);
    html.replace("%LCD_OTA%", globalData.otaHost);
    html.replace("%LCD_TEMP%", String(globalData.temperature, 1));
    // ---------------------------------
 
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
    bool use_builtin_led = server.arg("use_builtin_led") == "on";

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
    preferences.putBool("use_builtin_led", use_builtin_led);

    preferences.end();

    preferences.begin("wifi-config", false);
    String ssid1 = server.arg("ssid1");
    String password1 = server.arg("password1");
    String ssid2 = server.arg("ssid2");
    String password2 = server.arg("password2");
    String ssid3 = server.arg("ssid3");
    String password3 = server.arg("password3");

    preferences.putString("ssid1", ssid1);
    preferences.putString("password1", password1);
    preferences.putString("ssid2", ssid2);
    preferences.putString("password2", password2);
    preferences.putString("ssid3", ssid3);
    preferences.putString("password3", password3);
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
const unsigned long builtinLedCheckInterval = 500; // Проверка каждую секунду
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

void readSettings()
{
    preferences.begin("settings", true);
    useMQTTBuiltinLed = preferences.getBool("use_builtin_led", false);
    preferences.end();
}



void setup()
{
    SerialLog.begin(115200);
    delay(1000); // Даём время на инициализацию Serial
    SerialLog.println("Starting...");

    // Отключаем неиспользуемые функции для экономии памяти
    btStop(); // Отключаем Bluetooth

    readSettings(); // Читаем настройки при запуске

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

// Экран
#if LCD_ONBOARD
    lcdInit();
    delay(100);
#endif

// Лента
#if LED_ONBOARD
    strip.begin();
    strip.show();
    delay(100);
#endif

 
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
        lcdPrint("OTA Init");
        ArduinoOTA.setHostname(OTA_HOSTNAME);
        ArduinoOTA.begin();
        SerialLog.println("OTA Ready");

        ArduinoOTA.onStart([]() {
            otaRunning = true;
            SerialLog.println("Start updating...");
            cameraDeinit(); // важно
        });
        ArduinoOTA.onError([](ota_error_t error) {
            SerialLog.printf("OTA Error[%u]\n", error);
            otaRunning = false; // Сброс если ошибка
        });  
        
        ArduinoOTA.onEnd([]() {
            SerialLog.println("OTA End");
            ESP.restart(); // Перезагружаем устройство
          });

        // Устанавливаем OTA host в globalData
        globalData.otaHost = OTA_HOSTNAME;

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

        if (LED_ONBOARD)
        {
            server.on("/led_on", handleLEDOn);
            server.on("/led_off", handleLEDOff);
        }

        server.on("/serial-log", HTTP_GET, handleSerialLog);

        server.begin();

        lcdPrint("MQTT Init");
        SerialLog.println("MQTT Init");
        delay(100);
        mqttInit();
    }
}

void commonLoop()
{
    ArduinoOTA.handle();
    if (otaRunning) return;
    server.handleClient();
    mqttLoop();
#if LCD_ONBOARD
    if (getLcdState())
    {
        updateDisplay();
    }
#endif
#if TEMPERATURE_MQTT
    temperatureLoop();
#endif

    builtinLedStateLoop();
    cameraStateLoop();

    checkWiFiConnection();
}

void checkWifiAPLoop()
{
    static unsigned long lastCheckTime = 0; // Для проверки каждую секунду
    unsigned long now = millis();

    if (WifiAPStartTime != 0 && now - lastCheckTime >= 1000) // Проверяем каждую секунду
    {
        lastCheckTime = now;

        if (now - WifiAPStartTime > WifiAPTimeout)
        {
            SerialLog.println("AP mode timeout. Resetting AP flag and restarting...");
            preferences.begin("wifi-config", false);
            preferences.putBool("ap", false); // Сбрасываем флаг AP
            preferences.end();
            ESP.restart(); // Перезагружаем устройство
        }
    }
}

void loop()
{

    // dnsServer.processNextRequest();

    if (isWifiConnect != 1)
    {
        serverSettings.handleClient();
        checkWifiAPLoop(); // Проверяем таймаут режима AP
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

void handleStream()
{
    WiFiClient client = server.client();
    camera_fb_t *fb = NULL;
    SerialLog.println("Stream Start");
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
            SerialLog.println("Camera capture failed");
            break;
        }

        response = "--frame\r\n";
        response += "Content-Type: image/jpeg\r\n\r\n";
        client.write(response.c_str(), response.length());
        client.write(fb->buf, fb->len);
        client.write("\r\n", 2);

        esp_camera_fb_return(fb);

        // Обработка запросов сервера каждые 10 итераций
        if (counter++ % 5 == 0)
        {
            commonLoop();
        }
    }
    SerialLog.println("Stream End");
}

void handleSnapshot()
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        SerialLog.println("Camera capture failed");
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

void handleSerialLog() {
    String logContent = SerialLog.getLog(); // Assuming SerialLog has a method to retrieve the log buffer
    server.send(200, "text/plain", logContent); 
}

