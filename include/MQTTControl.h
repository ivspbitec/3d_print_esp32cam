#pragma once

#include "SerialLogger.h"
void mqttStateLed(const String &state, int led_r, int led_g, int led_b, int brightness);
void mqttTemperature(const String &state);
void mqttBuiltinLedState(const String &state); // <--- Добавлено объявление
 
//#include "DeviceConfig.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>

#include <LedControl.h>
#include <LcdControl.h>
#include "CameraSetup.h" 

WiFiClient espClient;
PubSubClient mqttClient(espClient);
extern Preferences preferences;
extern DisplayData globalData;

bool isMqttConnected = false;
bool isMqttDisabled = false;
int mqttTryes = 5;
String mqtt_user_global;

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    if (isMqttDisabled)
        return;

    SerialLog.println("MQTT Callback received");
    SerialLog.print("Topic: ");
    SerialLog.println(topic);
    SerialLog.print("Length: ");
    SerialLog.println(length);

    // Создаем строку из полученного сообщения
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    SerialLog.print("Message arrived [");
    SerialLog.print(topic);
    SerialLog.print("] ");
    SerialLog.println(message);

    lcdPrint("MQTT: %s", message.c_str());

    DynamicJsonDocument doc(512); // Задаем размер документа, 512 байт должно быть достаточно для большинства случаев
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        SerialLog.print("deserializeJson() failed: ");
        SerialLog.println(error.c_str());
         
    }

    String topicStr = String(topic);

   
    if (LED_ONBOARD && topicStr == String(mqtt_user_global+"/led/set"))
    {

        if (doc.containsKey("state"))
        {
            String state = doc["state"].as<String>();

            // if (doc.containsKey("brightness")) {
            // v   String state = doc.as<String>();
            if (state == "ON")
            {
                LedOn(); // Функция для включения ленты
                SerialLog.print("mqttCallback() /led/set on ");
            }
            else if (state == "OFF")
            { 
                LedOff(); // Функция для выключения ленты

                SerialLog.print("mqttCallback() /led/set off ");
            }
        }
        if (doc.containsKey("brightness"))
        {
            String brightness = doc["brightness"].as<String>();
            preferences.begin("settings", false);
            preferences.putString("brightness", brightness);
            preferences.end();

            SerialLog.print("mqttCallback() /led/set brightness ");
            
        }
        if (doc.containsKey("rgb"))
        {
            JsonArray rgb = doc["rgb"].as<JsonArray>();
            String led_r = rgb[0];
            String led_g = rgb[1];
            String led_b = rgb[2];
            // Сохраните значения цвета в ваших настройках или примените их напрямую
            preferences.begin("settings", false);
            preferences.putString("led_r", led_r);
            preferences.putString("led_g", led_g);
            preferences.putString("led_b", led_b);
            preferences.end();

            SerialLog.print("mqttCallback() /led/set rgb ");
 
        }
    } else if (topicStr == String(mqtt_user_global+"/cmd"))
    {
        if (message == "restart"){
            SerialLog.print("mqttCallback() /cmd/ restart ");
            lcdPrint("Restarting...");
            ESP.restart();
        }    
    } else if (topicStr == String(mqtt_user_global+"/led_builtin/set"))
    {
#if !defined(BUILTIN_LED_FORBID) || (BUILTIN_LED_FORBID == 0)
        if (useMQTTBuiltinLed){
            if (doc.containsKey("state"))
            {
                String state = doc["state"].as<String>();
    
                if (state == "ON")
                {   
                    digitalWrite(LED_BUILTIN, HIGH);
                    SerialLog.print("mqttCallback() /led_builtin/set on ");
                    mqttBuiltinLedState("ON"); // Публикуем новое состояние
                }
                else if (state == "OFF")
                { 
                    digitalWrite(LED_BUILTIN, LOW);
                    SerialLog.print("mqttCallback() /led_builtin/set off ");
                    mqttBuiltinLedState("OFF"); // Публикуем новое состояние
                }
            }
            // Обработка простого payload без JSON
            else if (message == "ON") {
                digitalWrite(LED_BUILTIN, HIGH);
                SerialLog.print("mqttCallback() /led_builtin/set on (raw payload) ");
                mqttBuiltinLedState("ON");
            }
            else if (message == "OFF") {
                digitalWrite(LED_BUILTIN, LOW);
                SerialLog.print("mqttCallback() /led_builtin/set off (raw payload) ");
                mqttBuiltinLedState("OFF");
            }
        }
#else
        Serial.println("BUILTIN LED usage is forbidden by build config.");
#endif
    } else if (topicStr == String(mqtt_user_global+"/camera/set"))
    {
        bool settingsChanged = false;
        preferences.begin("settings", false);
        
        if (doc.containsKey("resolution"))
        {
            SerialLog.print("mqttCallback() /camera/set ");
            int resolution;
            if (doc["resolution"].is<String>()) {
                resolution = doc["resolution"].as<String>().toInt();
            } else {
                resolution = doc["resolution"].as<int>();
            }
            int currentResolution = preferences.getInt("resolution", 0);
            if (currentResolution != resolution) {
                preferences.putInt("resolution", resolution);
                settingsChanged = true;
                SerialLog.print("mqttCallback() /camera/set resolution changed to ");
                SerialLog.println(resolution);
            }
        }
        if (doc.containsKey("quality"))
        {
            int quality;
            if (doc["quality"].is<String>()) {
                quality = doc["quality"].as<String>().toInt();
            } else {
                quality = doc["quality"].as<int>();
            }
            int currentQuality = preferences.getInt("camera_quality", 0);
            if (currentQuality != quality) {
                preferences.putInt("camera_quality", quality);
                settingsChanged = true;
                SerialLog.print("mqttCallback() /camera/set quality changed to ");
                SerialLog.println(quality);
            }
        }
        preferences.end();

        if (settingsChanged) {
           // Serial.println("Camera settings changed, restarting...");
           // delay(500);
           // ESP.restart();
            Serial.println("Camera settings changed, reinitializing camera...");
            lcdPrint("Reinit camera...");
  
            esp_camera_deinit();            
            cameraInit();
            lcdPrint("Camera done");               
        }
    }
}


int mqttConnectionTryes = 5;

// Функция повторного подключения к MQTT серверу
void mqttReconnect()
{
    if (isMqttDisabled)
        return;

    preferences.begin("settings", false);
    String mqtt_server = preferences.getString("mqtt_server", "");
    int mqtt_port = preferences.getInt("mqtt_port", 1883);
    String mqtt_user = preferences.getString("mqtt_user", "");
    String mqtt_password = preferences.getString("mqtt_password", "");
    preferences.end();

    mqtt_user_global=mqtt_user;

    if (mqtt_server == "")
        return;
    SerialLog.println("mqttReconnect");
    SerialLog.println(mqtt_server);
    SerialLog.println(mqtt_port);
    SerialLog.println(mqtt_user);
    SerialLog.println(mqtt_password);

    if (!mqttClient.connected())
    {
        SerialLog.print("Attempting MQTT connection...");

        /*if (mqttClient.connect("ESP32Client", mqtt_user.c_str(), mqtt_password.c_str()))*/
        if (mqttClient.connect(mqtt_user.c_str(), mqtt_user.c_str(), mqtt_password.c_str()))
        {
            SerialLog.println("connected");            
            globalData.isMqttConnected = true;
            isMqttConnected = true;
          
            mqttClient.subscribe((mqtt_user_global + "/led/set").c_str());
            mqttClient.subscribe((mqtt_user_global + "/led_builtin/set").c_str());
            mqttClient.subscribe((mqtt_user_global + "/cmd").c_str());
            mqttClient.subscribe((mqtt_user_global + "/camera/set").c_str());
            mqttConnectionTryes=10;
            
            mqttBuiltinLedState(digitalRead(LED_BUILTIN) == HIGH ? "ON" : "OFF");
            //  LedOff();
        }
        else
        {
            SerialLog.print("failed, rc=");
            SerialLog.print(mqttClient.state());
            SerialLog.println(" try again in 5 seconds");
            lcdPrint("MQTT failed");
            globalData.isMqttConnected = false;

            mqttTryes = mqttConnectionTryes - 1;
            SerialLog.println(mqttConnectionTryes);
            if (mqttConnectionTryes <= 0)
            {
                isMqttDisabled = true;            
            }
           
        }
    }
}

// Функция инициализации MQTT
void mqttInit()
{
    SerialLog.println("mqttInit");

    preferences.begin("settings", false);
    String mqtt_server = preferences.getString("mqtt_server", "");
    int mqtt_port = preferences.getInt("mqtt_port", 1883);
    preferences.end();
     globalData.isMqttConnected = false;

    if (!mqtt_server.isEmpty())
    {
        SerialLog.println("mqttClient setserver");
        SerialLog.println(mqtt_server.c_str());
        SerialLog.println(mqtt_port);

        IPAddress mqtt_ip;
        mqtt_ip.fromString(mqtt_server);
        SerialLog.println(mqtt_ip);

        if (!mqtt_ip.fromString(mqtt_server))
        {
            SerialLog.println("Invalid IP address format");
            mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
        }
        else
        {
            mqttClient.setServer(mqtt_ip, mqtt_port);
        }

        mqttClient.setKeepAlive(60); // Установить keep-alive интервал в 60 секунд
        mqttClient.setCallback(mqttCallback);
    }
    else
    {
        isMqttDisabled = true;
    }
}

// Переменная для хранения времени последней попытки подключения
unsigned long lastReconnectAttempt = 0;

// Интервал между попытками подключения (в миллисекундах)
const unsigned long reconnectInterval = 5000;


// Функция для обработки MQTT сообщений
void mqttLoop(){
     if (!isMqttDisabled){
        unsigned long now = millis();  // Текущее время

        //if (!isMqttConnected){
        if (!mqttClient.connected()){ 
            // Проверяем, прошло ли достаточно времени с последней попытки подключения
            if (now - lastReconnectAttempt >= reconnectInterval){
                // Обновляем время последней попытки
                lastReconnectAttempt = now;
                SerialLog.println("mqttLoop -> mqttReconnect()");
                
                mqttReconnect();
            }
        }
        mqttClient.loop();
    }
}

// Функции установки состояний

void mqttStateLed(const String &state, int led_r, int led_g, int led_b, int brightness)
{
    #if !LED_ONBOARD
    return;
    #endif

    if (isMqttDisabled || !mqttClient.connected())
        return;

    JsonDocument doc;
    doc["state"] = state;
    doc["brightness"] = brightness;
    JsonObject color = doc["color"].to<JsonObject>();
    color["r"] = led_r;
    color["g"] = led_g;
    color["b"] = led_b;

    char buffer[256];
    serializeJson(doc, buffer);

    mqttClient.publish(String(mqtt_user_global+"/led/state").c_str(), buffer);
}

// Функции установки состояний

void mqttTemperature(const String &state)
{
    if (isMqttDisabled || !mqttClient.connected())
        return;

    mqttClient.publish(String(mqtt_user_global+"/temperature/state").c_str(), state.c_str());
}

void mqttBuiltinLedState(const String &state)
{
 

    if (isMqttDisabled || !mqttClient.connected()) 
        return;

    StaticJsonDocument<64> doc;
    doc["state"] = state;

    char buffer[64];
    serializeJson(doc, buffer);

    mqttClient.publish(String(mqtt_user_global+"/led_builtin/state").c_str(), buffer);
}

void mqttCameraState(int resolution, int quality)
{
    if (isMqttDisabled || !mqttClient.connected()) 
        return;

    StaticJsonDocument<128> doc;
    doc["resolution"] = resolution;
    doc["quality"] = quality;

    char buffer[128];
    serializeJson(doc, buffer);

    mqttClient.publish(String(mqtt_user_global+"/camera/state").c_str(), buffer);
}