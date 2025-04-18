#pragma once

// Настройки дисплея
#define LCD_ONBOARD 1  // 1 - если есть встроенный LCD, 0 - если нет
#define DispSdaPin 15  // Пин SDA для дисплея
#define DispSclPin 13  // Пин SCL для дисплея 

// Настройки LED
#define LED_ONBOARD 0  // 1 - если есть встроенная LED лента, 0 - если нет
#define LED_PIN 2      // Пин для LED ленты
#define LED_COUNT 6    // Количество светодиодов в ленте
// Настройки пинов

//Передавать температуру в MQTT
#define TEMPERATURE_MQTT 0
  
 bool useMQTTBuiltinLed = false; // Глобальная переменная для настройки отправки MQTT сообщений о состоянии встроенного светодиода