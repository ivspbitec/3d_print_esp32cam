#pragma once

#include <Adafruit_NeoPixel.h>
#include "Adafruit_GFX.h"

#include "MQTTControl.h" // Подключение заголовочного файла с объявлением функции

extern Preferences preferences;

#ifndef LED_COUNT
#define LED_COUNT 6 // Количество светодиодов в ленте (замените на нужное значение)
#endif

#ifndef LED_PIN
#define LED_PIN 2 // Номер пина, к которому подключена лента (замените на нужное значение)
#endif

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

bool ledState = false; // Состояние светодиодов (включен/выключен)
bool ledBlock = false; // Состояние светодиодов (включен/выключен)

void LedOn(bool isMqttStateSent=true) {
    Serial.println("LedOn()");
    if (ledBlock) return;
    ledBlock=true;    
    preferences.begin("settings", true);
    int led_r = preferences.getString("led_r", "255").toInt();
    int led_g = preferences.getString("led_g", "255").toInt();
    int led_b = preferences.getString("led_b", "255").toInt();
    int brightness = preferences.getString("brightness", "255").toInt();
    preferences.end();

    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(led_r, led_g, led_b)); // Устанавливаем белый цвет
    }
    strip.setBrightness(brightness);
    strip.show();
    ledState = true;

    if (isMqttStateSent){
        mqttStateLed("ON", led_r, led_g, led_b, brightness);
    }
    
    ledBlock=false;
}

void LedOff() {
        if (ledBlock) return;
    ledBlock=true;    
    Serial.println("LedOff()");
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(0, 0, 0)); // Выключаем светодиоды
    }
    strip.show();
    ledState = false;

    mqttStateLed("OFF", 0, 0, 0, 0);
    ledBlock=false;
}

bool getLedState() {
    return ledState;
}

/*
void setLedBrightness(int brightness) {
   Serial.println("applyBrightness()");
   strip.setBrightness(brightness);
   strip.show();
}

void setLedColor(int r, int g, int b) {
    Serial.println("setLedColor()");
     for(int i = 0; i < strip.numPixels(); i++) {
       strip.setPixelColor(i, strip.Color(r, g, b));
   }
   strip.show();
}*/