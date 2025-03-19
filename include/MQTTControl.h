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
String mqtt_user_global;

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    if (isMqttDisabled)
        return;

    Serial.println("MQTT Callback received");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Length: ");
    Serial.println(length);

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
         
    }

    String topicStr = String(topic);

    if (topicStr == String(mqtt_user_global+"/led/set"))
    {

        if (doc.containsKey("state"))
        {
            String state = doc["state"].as<String>();

            // if (doc.containsKey("brightness")) {
            // v   String state = doc.as<String>();
            if (state == "ON")
            {
                LedOn(); // Функция для включения ленты
                Serial.print("mqttCallback() /led/set on ");
            }
            else if (state == "OFF")
            { 
                LedOff(); // Функция для выключения ленты

                Serial.print("mqttCallback() /led/set off ");
            }
        }
        if (doc.containsKey("brightness"))
        {
            String brightness = doc["brightness"].as<String>();
            preferences.begin("settings", false);
            preferences.putString("brightness", brightness);
            preferences.end();

            Serial.print("mqttCallback() /led/set brightness ");
            
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

            Serial.print("mqttCallback() /led/set rgb ");
 
        }
    } else if (topicStr == String(mqtt_user_global+"/cmd"))
    {
        if (message == "restart"){
            Serial.print("mqttCallback() /cmd/ restart ");
            lcdPrint("Restarting...");
            ESP.restart();
        }    
    } else if (topicStr == String(mqtt_user_global+"/led_builtin/set"))
    {
        if (doc.containsKey("state"))
        {
            String state = doc["state"].as<String>();
 
            if (state == "ON")
            {
                digitalWrite(LED_BUILTIN, HIGH);
                 Serial.print("mqttCallback() /led_builtin/set on ");
            }
            else if (state == "OFF")
            { 
                digitalWrite(LED_BUILTIN, LOW);
                Serial.print("mqttCallback() /led_builtin/set off ");
            }
        }
    } else if (topicStr == String(mqtt_user_global+"/camera/set"))
    {
        if (doc.containsKey("resolution"))
        {
            Serial.print("mqttCallback() /camera/set ");
            int resolution;
            if (doc["resolution"].is<String>()) {
                resolution = doc["resolution"].as<String>().toInt();
            } else {
                resolution = doc["resolution"].as<int>();
            }
            preferences.begin("settings", false);
            preferences.putInt("resolution", resolution);
            preferences.end();
            Serial.print("mqttCallback() /camera/set resolution ");
            Serial.println(resolution);
        }
        if (doc.containsKey("quality"))
        {
            int quality;
            if (doc["quality"].is<String>()) {
                quality = doc["quality"].as<String>().toInt();
            } else {
                quality = doc["quality"].as<int>();
            }
            preferences.begin("settings", false); 
            preferences.putInt("camera_quality", quality);
            preferences.end();
            Serial.print("mqttCallback() /camera/set quality ");
            Serial.println(quality);
        }

        if (doc.containsKey("resolution") || doc.containsKey("quality")){
            delay(500);
            ESP.restart();
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
    Serial.println("mqttReconnect");
    Serial.println(mqtt_server);
    Serial.println(mqtt_port);
    Serial.println(mqtt_user);
    Serial.println(mqtt_password);

    if (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");

        /*if (mqttClient.connect("ESP32Client", mqtt_user.c_str(), mqtt_password.c_str()))*/
        if (mqttClient.connect(mqtt_user.c_str(), mqtt_user.c_str(), mqtt_password.c_str()))
        {
            Serial.println("connected");            
            globalData.isMqttConnected = true;
            isMqttConnected = true;
          
            mqttClient.subscribe((mqtt_user_global + "/led/set").c_str());
            mqttClient.subscribe((mqtt_user_global + "/led_builtin/set").c_str());
            mqttClient.subscribe((mqtt_user_global + "/cmd").c_str());
            mqttClient.subscribe((mqtt_user_global + "/camera/set").c_str());
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

        //if (!isMqttConnected){
        if (!mqttClient.connected()){ 
            // Проверяем, прошло ли достаточно времени с последней попытки подключения
            if (now - lastReconnectAttempt >= reconnectInterval){
                // Обновляем время последней попытки
                lastReconnectAttempt = now;
                Serial.println("mqttLoop -> mqttReconnect()");
                
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
    mqttClient.publish(String(mqtt_user_global+"/led/state").c_str(), buffer);
}

// Функции установки состояний

void mqttTemperature(const String &state)
{
    if (isMqttDisabled)
        return;
    if (!isMqttConnected)
        return;

    mqttClient.publish(String(mqtt_user_global+"/temperature/state").c_str(), state.c_str());
}



void mqttBuiltinLedState(const String &state)
{
    if (isMqttDisabled || !mqttClient.connected()) return;
    StaticJsonDocument<64> doc;
    doc["state"] = state;

    char buffer[64];
    serializeJson(doc, buffer);

    mqttClient.publish(String(mqtt_user_global+"/led_builtin/state").c_str(), buffer);
}

void mqttCameraState(int resolution, int quality)
{
    if (isMqttDisabled || !mqttClient.connected()) return;
    StaticJsonDocument<128> doc;
    doc["resolution"] = resolution;
    doc["quality"] = quality;

    char buffer[128];
    serializeJson(doc, buffer);

    mqttClient.publish(String(mqtt_user_global+"/camera/state").c_str(), buffer);
}