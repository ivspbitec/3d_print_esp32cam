#pragma once

unsigned long lastUpdate = 0; // Время последнего обновления дисплея
const unsigned long updateInterval = 1000; // Интервал обновления в миллисекундах
extern DisplayData globalData;

int isLCD=0;

#ifndef DispSdaPin
#define DispSdaPin 15
#endif

#ifndef DispSclPin
#define DispSclPin 13
#endif
 
 

// библиотеки для работы с OLED экраном Arduino IDE
#include "Wire.h"
#include "Adafruit_SSD1306.h"
#include "MQTTControl.h"
#include "SerialLogger.h"

Adafruit_SSD1306 display(128, 32, &Wire, -1); // указываем размер экрана в пикселях


bool getLcdState(){
   if (isLCD!=1){
        return false;
    } else{
        return true;
    }
}

void lcdPrint(const char *format, ...) {
    if (isLCD!=1){
        return;
    }
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
        SerialLog.println(F("SSD1306 allocation failed"));
        // for(;;); // Don't proceed, loop forever
        return;
    }
    isLCD=1;
    SerialLog.println("LCD Init");   
    display.clearDisplay(); 
    lcdPrint("LCD Init");
}

void updateDisplay() {
    // Проверяем, прошло ли достаточно времени с последнего обновления
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= updateInterval) {
        lastUpdate = currentMillis;

        display.clearDisplay();
        display.setTextSize(1, 1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.printf(
            "W:%s M:%s %s %s OTA:%s T:%.1f",
            (globalData.isWifiConnected ? "*" : "-"),
            (globalData.isMqttConnected ? "*" : "-"),
            globalData.ssid.c_str(),
            globalData.ip.c_str(),
            globalData.otaHost.c_str(),
            globalData.temperature
        );
        display.display();

        // Если нужно оставить старый lcdPrint, закомментируйте или удалите его:
        // lcdPrint("Wifi: %s, MQTT: %s, SSID: %s, IP: %s,  t: %.2f C",
        //          (globalData.isWifiConnected ? "*" : ""),
        //          (globalData.isMqttConnected ? "*" : ""),
        //          globalData.ssid.c_str(),
        //          globalData.ip.c_str(),
        //          globalData.temperature
        //          );
    }
}