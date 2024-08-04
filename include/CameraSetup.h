#pragma once

#include <esp_camera.h>
#include "CameraSetup.h"
#include <Arduino.h>

// Настройки для камеры
#define CAMERA_MODEL_AI_THINKER

// Пин конфигурация для AI Thinker модели
#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#endif

/*FRAMESIZE_UXGA (1600 x 1200)
FRAMESIZE_QVGA (320 x 240)
FRAMESIZE_CIF (352 x 288)
FRAMESIZE_VGA (640 x 480)
FRAMESIZE_SVGA (800 x 600)
FRAMESIZE_XGA (1024 x 768)
FRAMESIZE_SXGA (1280 x 1024)*/
void cameraInit()
{
    // esp_camera_deinit();

    // Инициализация камеры
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    // config.pixel_format = PIXFORMAT_JPEG;
    // config.pixel_format = PIXFORMAT_RGB565;

    preferences.begin("settings", true);
    int resolution = preferences.getInt("resolution", FRAMESIZE_SVGA);
    bool flip_vertical = preferences.getBool("flip_vertical", false);
    int camera_quality = preferences.getInt("camera_quality", 12);
    preferences.end();

   //  config.frame_size = FRAMESIZE_SVGA; // 640x480
    //config.frame_size = FRAMESIZE_VGA; // 640x480
    config.frame_size = static_cast<framesize_t>(resolution); // 640x480
    config.jpeg_quality = camera_quality;
    //config.jpeg_quality = 12;
    config.fb_count = 1;

    Serial.println(camera_quality);
    Serial.println(resolution);
    Serial.println(flip_vertical);

    // config.frame_size = FRAMESIZE_96X96; // 640x480

    // Инициализация камеры
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }



    // Переворот изображения по вертикали
    if (flip_vertical){
        sensor_t *s = esp_camera_sensor_get();
        if (s == NULL)
        {
            Serial.println("Failed to get camera sensor settings");
            return;
        }

        s->set_vflip(s, 1); // 1 для включения переворота, 0 для отключения
        s->set_hmirror(s, 1);
    }

    Serial.println("Camera reinitialized successfully");
}
