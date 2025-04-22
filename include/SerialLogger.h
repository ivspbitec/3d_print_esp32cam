#pragma once
#include <Arduino.h>

#define LOG_LINES 50

unsigned long startTime = millis(); // Время старта устройства

void addTimestamp(String &logEntry) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - startTime) / 1000; // Время в секундах
    unsigned long hours = elapsedTime / 3600;
    unsigned long minutes = (elapsedTime % 3600) / 60;
    unsigned long seconds = elapsedTime % 60;

    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "[%02lu:%02lu:%02lu]", hours, minutes, seconds);
    logEntry = String(timestamp) + " " + logEntry;
}

class SerialLogger : public Stream {
public:
    void begin(unsigned long speed) {
        Serial.begin(speed);
    }

    // Print methods
    template<typename T>
    size_t print(const T& msg) {
        size_t n = Serial.print(msg);
        addToBuffer(String(msg), false);
        return n;
    }

    size_t print(const __FlashStringHelper *msg) {
        size_t n = Serial.print(msg);
        addToBuffer(String(msg), false);
        return n;
    }

    size_t print(char c) {
        size_t n = Serial.print(c);
        addToBuffer(String(c), false);
        return n;
    }

    // Println methods
    template<typename T>
    size_t println(const T& msg) {
        size_t n = Serial.println(msg);
        addToBuffer(String(msg));
        return n;
    }

    size_t println(const __FlashStringHelper *msg) {
        size_t n = Serial.println(msg);
        addToBuffer(String(msg));
        return n;
    }

    size_t println() {
        size_t n = Serial.println();
        addToBuffer("");
        return n;
    }

    // Write methods
    size_t write(uint8_t c) override {
        size_t n = Serial.write(c);
        addToBuffer(String((char)c), false);
        return n;
    }

    size_t write(const uint8_t *buffer, size_t size) override {
        size_t n = Serial.write(buffer, size);
        for (size_t i = 0; i < size; ++i) {
            addToBuffer(String((char)buffer[i]), false);
        }
        return n;
    }

    int available() override { return Serial.available(); }
    int read() override { return Serial.read(); }
    int peek() override { return Serial.peek(); }
    void flush() override { Serial.flush(); }

    // Получить лог как одну строку (например, для веб-интерфейса)
    String getLog(int lines = LOG_LINES) const {
        String out;
        int idx = logIndex;
        for (int i = 0; i < lines; i++) {
            out += logBuffer[idx] + "\n";
            idx = (idx + 1) % LOG_LINES;
        }
        return out;
    }

    // Вывести лог в Serial
    void printLog(int lines = LOG_LINES) const {
        int idx = logIndex;
        for (int i = 0; i < lines; i++) {
            Serial.println(logBuffer[idx]);
            idx = (idx + 1) % LOG_LINES;
        }
    }

private:
    String logBuffer[LOG_LINES];
    int logIndex = 0;
    bool lastWasNewline = true;

    void addToBuffer(const String& msg, bool newline = true) {
        String logEntry = msg;
        addTimestamp(logEntry); // Добавляем метку времени

        if (newline || lastWasNewline) {
            logBuffer[logIndex] = logEntry;
            logIndex = (logIndex + 1) % LOG_LINES;
            lastWasNewline = newline;
        } else {
            logBuffer[(logIndex - 1 + LOG_LINES) % LOG_LINES] += logEntry;
        }
        lastWasNewline = newline;
    } 
};

