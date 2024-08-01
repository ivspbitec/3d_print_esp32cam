#pragma once

void mqttStateLed(const String &state, int led_r, int led_g, int led_b, int brightness);
void mqttTemperature(const String &state);

#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <LedControl.h>
#include <LcdControl.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);
extern Preferences preferences;
extern DisplayData globalData;

bool isMqttConnected = false;
bool isMqttDisabled = false;
int mqttTryes = 5;

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    if (isMqttDisabled)
        return;

    // Создаем строку из полученного сообщения
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(message);

    lcdPrint("MQTT: %s", message.c_str());

    DynamicJsonDocument doc(512); // Задаем размер документа, 512 байт должно быть достаточно для большинства случаев
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    String topicStr = String(topic);

    if (topicStr == "esp32/led/set")
    {

        if (doc.containsKey("state"))
        {
            String state = doc["state"].as<String>();

            // if (doc.containsKey("brightness")) {
            // v   String state = doc.as<String>();
            if (state == "ON")
            {
                LedOn(); // Функция для включения ленты
            }
            else if (state == "OFF")
            { 
                LedOff(); // Функция для выключения ленты
            }
        }
        if (doc.containsKey("brightness"))
        {
            String brightness = doc["brightness"].as<String>();
            preferences.begin("settings", false);
            preferences.putString("brightness", brightness);
            preferences.end();
            
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
 
        }
    }
}


int mqttConnectionTryes = 10;

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

    if (mqtt_server == "")
        return;
    Serial.println("mqttReconnect");
    Serial.println(mqtt_server);
    Serial.println(mqtt_port);
    Serial.println(mqtt_user);
    Serial.println(mqtt_password);

    if (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");

        if (mqttClient.connect("ESP32Client", mqtt_user.c_str(), mqtt_password.c_str()))
        {
            Serial.println("connected");            
            globalData.isMqttConnected = true;
            isMqttConnected = true;
          
            mqttClient.subscribe("esp32/led/set");
            mqttConnectionTryes=10;

            //  LedOff();
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            lcdPrint("MQTT failed");
            globalData.isMqttConnected = false;

            mqttTryes = mqttConnectionTryes - 1;
            Serial.println(mqttConnectionTryes);
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
    Serial.println("mqttInit");

    preferences.begin("settings", false);
    String mqtt_server = preferences.getString("mqtt_server", "");
    int mqtt_port = preferences.getInt("mqtt_port", 1883);
    preferences.end();
     globalData.isMqttConnected = false;

    if (!mqtt_server.isEmpty())
    {
        Serial.println("mqttClient setserver");
        Serial.println(mqtt_server.c_str());
        Serial.println(mqtt_port);

        IPAddress mqtt_ip;
        mqtt_ip.fromString(mqtt_server);
        Serial.println(mqtt_ip);

        if (!mqtt_ip.fromString(mqtt_server))
        {
            Serial.println("Invalid IP address format");
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
const unsigned long reconnectInterval = 1000;  // 2 секунды


// Функция для обработки MQTT сообщений
void mqttLoop(){
     if (!isMqttDisabled){
        unsigned long now = millis();  // Текущее время

        if (!mqttClient.connected()){
            // Проверяем, прошло ли достаточно времени с последней попытки подключения
            if (now - lastReconnectAttempt >= reconnectInterval){
                // Обновляем время последней попытки
                lastReconnectAttempt = now;
                mqttReconnect();
            }
        }
        mqttClient.loop();
    }
}

// Функции установки состояний

void mqttStateLed(const String &state, int led_r, int led_g, int led_b, int brightness)
{
    if (isMqttDisabled)
        return;
    if (!isMqttConnected)
        return;
    // Создаем JSON документ
    JsonDocument doc;

    doc["state"] = state;
    doc["brightness"] = brightness;
    // JsonObject color = doc.createNestedObject("color");
    JsonObject color = doc["color"].to<JsonObject>();
    color["r"] = led_r;
    color["g"] = led_g;
    color["b"] = led_b;

    char buffer[256];
    serializeJson(doc, buffer);

    // Публикуем сообщение в формате JSON
    mqttClient.publish("esp32/led/state", buffer);
}

// Функции установки состояний

void mqttTemperature(const String &state)
{
    if (isMqttDisabled)
        return;
    if (!isMqttConnected)
        return;

    mqttClient.publish("esp32/temperature/state", state.c_str());
}