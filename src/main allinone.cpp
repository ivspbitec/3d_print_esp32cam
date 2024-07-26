#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_camera.h>
#include <Adafruit_NeoPixel.h>
#include "esp_system.h"
 

// Настройки для камеры
#define CAMERA_MODEL_AI_THINKER

const char* apSSID = "ESP_32CM";
const char* apPassword = "987654321S";

WebServer server(8080);
WebServer serverSettings(80);
Preferences preferences;
//#include <DNSServer.h>
//DNSServer dnsServer;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);


int isWifiConnect=0;
int isScreenCapture=0;
int isLCD=0;

// Пин конфигурация для AI Thinker модели
#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25 
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22
#endif

#define LED_PIN 2
#define LED_COUNT 6

#define DispSdaPin 15
#define DispSclPin 13



Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


// библиотеки для работы с OLED экраном Arduino IDE
#include "Wire.h"
#include "Adafruit_GFX.h" 
#include "Adafruit_SSD1306.h"


Adafruit_SSD1306 display(128, 32, &Wire, -1); // указываем размер экрана в пикселях


void handleStream();
void handleSnapshot();    
void handleLEDOn();
void handleLEDOff();
void handleCaptureOn();
void handleCaptureOff();

void captureAndDisplay();

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

    if (wifiConnect(ssid,password)){
        String redirectPage = "<html><head>";
        redirectPage += "<meta http-equiv='refresh' content='6;url=http://" + WiFi.localIP().toString() + "/stream'>";
        redirectPage += "</head><body>Saved and connected! The device will now restart.</body></html>";

        Serial.println(WiFi.localIP().toString());
        serverSettings.send(200, "text/html", redirectPage);
    }else{
        serverSettings.sendHeader("Location", "/", true); // Перенаправление на главную страницу
        serverSettings.send(302, "text/plain", "");
        wifiAP();
    }

    delay(1000); // Wait a bit before restarting
    ESP.restart();
}


void lcd(){

  Wire.begin(DispSdaPin,DispSclPin);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address SclPin for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
    Serial.println("LCD Init");

   display.clearDisplay();
   display.setTextSize(1, 1); // указываем размер шрифта
   display.setTextColor(SSD1306_WHITE); // указываем цвет надписи

   display.setCursor(0, 0);
   display.println(WiFi.SSID());
   display.setCursor(0, 16);
   display.println(WiFi.localIP());
   display.display();
   delay(3000);
}

void handleNotFound() {
    serverSettings.sendHeader("Location", "/", true); // Перенаправление на главную страницу
    serverSettings.send(302, "text/plain", "");
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
        Serial.println(F("SSD1306 allocation failed"));
        // for(;;); // Don't proceed, loop forever
        return;
    }
    isLCD=1;
    Serial.println("LCD Init");   
    display.clearDisplay(); 
    lcdPrint("LCD Init");
}


void cameraInit(pixformat_t pixel_format = PIXFORMAT_JPEG, framesize_t frame_size = FRAMESIZE_VGA) {
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
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = pixel_format;
    //config.pixel_format = PIXFORMAT_JPEG;
    //config.pixel_format = PIXFORMAT_RGB565; 

    if(psramFound()){
        config.frame_size = frame_size; // 640x480 (уменьшенное разрешение для снижения потребления памяти)
       // config.frame_size = FRAMESIZE_VGA; // 640x480 (уменьшенное разрешение для снижения потребления памяти)
        config.jpeg_quality = 12; // Увеличено значение для снижения качества изображения и потребления памяти
        config.fb_count = 2;
    } else {
        config.frame_size = frame_size; // 640x480
       // config.frame_size = frame_size; // 640x480
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

       // config.frame_size = FRAMESIZE_96X96; // 640x480

    // Инициализация камеры
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    Serial.println("Camera reinitialized successfully");
  
}


bool wifiConnect(String ssid, String password){
  // Подключение к сохраненной сети Wi-Fi
        isWifiConnect=0;
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.println("Connecting to saved WiFi...");
         
        unsigned long startTime = millis();

        while (WiFi.status() != WL_CONNECTED) { 
            delay(1000); 
            Serial.println("Connecting...");
            lcdPrint("Connecting...");
            if (millis() - startTime > 10000) { // Попытка подключения не более 10 секунд
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

          float temperature = (float)temperatureRead() / 100.0;
          

            lcdPrint("Wifi Connected. SSID: %s, IP: %s, RSSI: %d dBm, t: %.2f °C",
             WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI(), temperature);
             isWifiConnect=1;
             return true;
        }
    return false;    
}

void wifiAP(){
       //WiFi.softAPdisconnect(true);
      // WiFi.disconnect();
      
        WiFi.mode(WIFI_AP);
        WiFi.softAP(apSSID, apPassword);

       // dnsServer.start(DNS_PORT, "*", apIP);

        IPAddress apIP = WiFi.softAPIP();
        Serial.print("Access Point IP address: ");
        Serial.println(apIP);
        lcdPrint("Access Point SSID: %s, Password: %s, IP: %s", apSSID, apPassword, apIP.toString().c_str());
}

void wifiInit(){

    // Проверка сохраненных настроек Wi-Fi
    preferences.begin("wifi-config", true);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();

    if (ssid != "") {
      wifiConnect(ssid,password);
    }     
    if (WiFi.status() != WL_CONNECTED){
        wifiAP();
    }
}


void handleReset(){
    preferences.begin("wifi-config", false);
    preferences.clear(); 
    preferences.end();
    preferences.begin("settings", false);
    preferences.clear(); 
    preferences.end();
}

void handleSettings(){    
 
    preferences.begin("settings", true);
    String led_r = preferences.getString("led_r", "255");
    String led_g = preferences.getString("led_g", "255");
    String led_b = preferences.getString("led_b", "255");

    String camera_width = preferences.getString("camera_width", "");
    String camera_height = preferences.getString("camera_height", "");
    preferences.end();    

    server.send(200, "text/html", "<form method='POST' action='/save'>"
    "<h2>Led</h2><br>"
    "<label>R: <input name='led_r' value='"+led_r+"'></label><br>"
    "<label>G: <input name='led_g' value='"+led_g+"'></label><br>"
    "<label>B: <input name='led_b' value='"+led_b+"'></label><br><br>"
    "<h2>Camera</h2><br>"
    "<label>Width: <input name='led_g' value='"+camera_width+"'></label><br>"
    "<label>Height: <input name='led_g' value='"+camera_height+"'></label><br>"
    "<input type='submit'></form>"
    );


}

void handleSettingsSave(){
    String led_r = server.arg("led_r");
    String led_g = server.arg("led_g");
    String led_b = server.arg("led_b");

    String camera_height = server.arg("camera_height");
    String camera_width = server.arg("camera_width");
 
    preferences.begin("settings", false);
    preferences.putString("led_r", led_r);
    preferences.putString("led_g", led_g);
    preferences.putString("led_b", led_b);
    preferences.putString("camera_width", camera_width);
    preferences.putString("camera_height", camera_height);
    preferences.end();

    server.sendHeader("Location", "/settings", true);
    server.send(303); // Код состояния 303 See Other
}

void setup() {
    Serial.begin(115200);
    lcdInit();

    // Инициализация светодиодов
    strip.begin();
    strip.show(); // Инициализация всех светодиодов как "выключенные"

    wifiInit();
 
        if (isWifiConnect!=1){
            serverSettings.on("/",handleRoot);
            serverSettings.on("/save", HTTP_POST, handleSave);
            serverSettings.onNotFound(handleNotFound); 
            serverSettings.begin();
        }else{
            cameraInit();
            server.on("/stream", HTTP_GET, handleStream);
            server.on("/snapshot", HTTP_GET, handleSnapshot);
            server.on("/reset", HTTP_GET, handleReset);
            server.on("/settings", HTTP_GET, handleSettings);
            server.on("/save", HTTP_POST, handleSettingsSave);

            server.on("/", HTTP_GET, [](){

                if (server.arg("action") == "stream") {
                    handleStream();
                } else  if (server.arg("action") == "snapshot") {
                    handleSnapshot();
                }  
            });

            
            server.on("/led_on", handleLEDOn);
            server.on("/led_off", handleLEDOff);
            server.on("/capture_on", handleCaptureOn);
            server.on("/capture_off", handleCaptureOff);
            
            server.begin(); 

        }

}

void loop() {
     
   
    //dnsServer.processNextRequest();

    if (isWifiConnect!=1){          
        serverSettings.handleClient();
    }else{
        server.handleClient();
    }

    if (isScreenCapture==1){
        captureAndDisplay();
    }
}

void handleLEDOn() {
    preferences.begin("settings", true);
    int led_r = preferences.getString("led_r", "255").toInt();
    int led_g = preferences.getString("led_g", "255").toInt();
    int led_b = preferences.getString("led_b", "255").toInt();
    preferences.end();    

    for(int i=0; i<strip.numPixels(); i++) { 
        strip.setPixelColor(i, strip.Color(led_r, led_g, led_b)); // Устанавливаем белый цвет
    }
    strip.show();
    server.send(200, "text/html", "LEDs are now ON");
}

void handleLEDOff() {
    for(int i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0)); // Выключаем светодиоды
    }
    strip.show();
    server.send(200, "text/html", "LEDs are now OFF");
}


void handleCaptureOn() {
     lcdPrint("Capture ON");
     isScreenCapture=1;
     server.send(200, "text/html", "Capture are now ON");
    cameraInit(PIXFORMAT_RGB565,FRAMESIZE_96X96);

     delay(2000);
}

void handleCaptureOff() {
    lcdPrint("Capture OFF");
    isScreenCapture=0;
    server.send(200, "text/html", "Capture are now OFF");    
    cameraInit();

    delay(2000);
}

// Функция для настройки сервера вещания с камеры
void startCameraServer() {
    server.on("/stream", HTTP_GET, [](){
    //server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request){
        WiFiClient client = server.client();
        camera_fb_t * fb = NULL;
        Serial.println("Stream Start");
        if(!client.connected()){
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
                iterationCount=0;
            }
        
        }
        Serial.println("Stream End");
    });
}

void captureAndDisplay() {
     camera_fb_t *fb = NULL; // Указатель на структуру кадра камеры
    fb = esp_camera_fb_get(); // Получаем кадр с камеры

    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    // Отображение RGB565 изображения на OLED дисплее
    display.clearDisplay();
    for (int16_t y = 0; y < fb->height; y++) {
        for (int16_t x = 0; x < fb->width; x++) {
            // Получаем цвет пикселя в формате RGB565
            uint16_t pixel = fb->buf[y * fb->width + x];
  // Serial.println(pixel);
            // Определяем яркость пикселя (выбираем белый или черный)
          //  uint8_t brightness = ((pixel >> 8) & 0xFF) > 127 ? SSD1306_WHITE : 0;
            uint8_t brightness = pixel < 127 ? SSD1306_WHITE : SSD1306_BLACK;
//Serial.println(brightness);            
            // Отображаем пиксель на дисплее
            if (x<120 && y<60)
            display.drawPixel(x, y, brightness);       
//delay(500);            
       }
    }
    display.display(); // Отображаем на дисплее

    // Освобождаем кадр после использования
    esp_camera_fb_return(fb);
}

void handleStream() {
    WiFiClient client = server.client();
    camera_fb_t * fb = NULL;
    Serial.println("Stream Start");
    if (!client.connected()) {
        return;
    }
    String response = "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    client.write(response.c_str(), response.length());

    int counter = 0;
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

        // Обработка запросов сервера каждые 10 итераций
        if (counter++ % 10 == 0) {
            server.handleClient();
        }
    }
    Serial.println("Stream End");
}

void handleSnapshot() {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        server.send(500, "text/plain", "Camera capture failed");
        return;
    }

    WiFiClient client = server.client();
    if (!client.connected()) {
        esp_camera_fb_return(fb);
        return;
    }


        String response = "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\n Content-Length: " + String(fb->len) + "\r\n\r\n";
      
        client.write(response.c_str(), response.length());
        client.write(fb->buf, fb->len);
        client.write("\r\n", 2);
  

    esp_camera_fb_return(fb);
}