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
    /**
     * Инициализирует последовательный порт с заданной скоростью.
     * 
     * @param speed Скорость последовательного порта в бодах.
     */
    void begin(unsigned long speed) {
        Serial.begin(speed);
    }

    /**
     * Выводит сообщение в последовательный порт и добавляет его в лог.
     * 
     * @tparam T Тип сообщения.
     * @param msg Сообщение для вывода.
     * @return Количество выведенных символов.
     */
    template<typename T>
    size_t print(const T& msg) {
        size_t n = Serial.print(msg);
        addToBuffer(String(msg), false);
        return n;
    }

    /**
     * Выводит сообщение из флэш-памяти в последовательный порт и добавляет его в лог.
     * 
     * @param msg Сообщение для вывода.
     * @return Количество выведенных символов.
     */
    size_t print(const __FlashStringHelper *msg) {
        size_t n = Serial.print(msg);
        addToBuffer(String(msg), false);
        return n;
    }

    /**
     * Выводит символ в последовательный порт и добавляет его в лог.
     * 
     * @param c Символ для вывода.
     * @return Количество выведенных символов.
     */
    size_t print(char c) {
        size_t n = Serial.print(c);
        addToBuffer(String(c), false);
        return n;
    }

    /**
     * Выводит сообщение с переводом строки в последовательный порт и добавляет его в лог.
     * 
     * @tparam T Тип сообщения.
     * @param msg Сообщение для вывода.
     * @return Количество выведенных символов.
     */
    template<typename T>
    size_t println(const T& msg) {
        size_t n = Serial.println(msg);
        addToBuffer(String(msg));
        return n;
    }

    /**
     * Выводит сообщение из флэш-памяти с переводом строки в последовательный порт и добавляет его в лог.
     * 
     * @param msg Сообщение для вывода.
     * @return Количество выведенных символов.
     */
    size_t println(const __FlashStringHelper *msg) {
        size_t n = Serial.println(msg);
        addToBuffer(String(msg));
        return n;
    }

    /**
     * Выводит перевод строки в последовательный порт и добавляет его в лог.
     * 
     * @return Количество выведенных символов.
     */
    size_t println() {
        size_t n = Serial.println();
        addToBuffer("");
        return n;
    }

    /**
     * Записывает байт в последовательный порт и добавляет его в лог.
     * 
     * @param c Байт для записи.
     * @return Количество записанных байт.
     */
    size_t write(uint8_t c) override {
        size_t n = Serial.write(c);
        addToBuffer(String((char)c), false);
        return n;
    }

    /**
     * Записывает массив байт в последовательный порт и добавляет его в лог.
     * 
     * @param buffer Указатель на массив байт.
     * @param size Размер массива.
     * @return Количество записанных байт.
     */
    size_t write(const uint8_t *buffer, size_t size) override {
        size_t n = Serial.write(buffer, size);
        for (size_t i = 0; i < size; ++i) {
            addToBuffer(String((char)buffer[i]), false);
        }
        return n;
    }

    /**
     * Возвращает количество доступных байт для чтения из последовательного порта.
     * 
     * @return Количество доступных байт.
     */
    int available() override { return Serial.available(); }

    /**
     * Читает байт из последовательного порта.
     * 
     * @return Прочитанный байт или -1, если данных нет.
     */
    int read() override { return Serial.read(); }

    /**
     * Возвращает следующий байт из последовательного порта, не удаляя его из буфера.
     * 
     * @return Следующий байт или -1, если данных нет.
     */
    int peek() override { return Serial.peek(); }

    /**
     * Очищает буфер последовательного порта.
     */
    void flush() override { Serial.flush(); }

    /**
     * Возвращает лог в виде строки.
     * 
     * @param lines Количество строк лога для возврата.
     * @return Лог в виде строки.
     */
    String getLog(int lines = LOG_LINES) const {
        String out;
        int idx = logIndex;
        for (int i = 0; i < lines; i++) {
            out += logBuffer[idx] + "\n";
            idx = (idx + 1) % LOG_LINES;
        }
        return out;
    }

    /**
     * Выводит лог в последовательный порт.
     * 
     * @param lines Количество строк лога для вывода.
     */
    void printLog(int lines = LOG_LINES) const {
        int idx = logIndex;
        for (int i = 0; i < lines; i++) {
            Serial.println(logBuffer[idx]);
            idx = (idx + 1) % LOG_LINES;
        }
    }

    /**
     * Возвращает только новые строки, накопившиеся с момента последнего вызова.
     * Если запись в буфер уже пошла сначала, строки возвращаются в правильном порядке.
     * 
     * @return Новые строки лога в виде одной строки с разделением по \n.
     */
    String getNewLogLines() {
        String newLog;
        static int lastReadIndex = logIndex; // Индекс последнего прочитанного сообщения

        // Если logIndex обернулся вокруг, нужно обработать обе части буфера
        if (lastReadIndex > logIndex) {
            // Сначала читаем строки от lastReadIndex до конца буфера
            for (int i = lastReadIndex; i < LOG_LINES; i++) {
                if (!logBuffer[i].isEmpty()) {
                    newLog += logBuffer[i] + "\n";
                }
            }
            // Затем читаем строки от начала буфера до logIndex
            for (int i = 0; i < logIndex; i++) {
                if (!logBuffer[i].isEmpty()) {
                    newLog += logBuffer[i] + "\n";
                }
            }
        } else {
            // Если logIndex не обернулся, читаем строки от lastReadIndex до logIndex
            for (int i = lastReadIndex; i < logIndex; i++) {
                if (!logBuffer[i].isEmpty()) {
                    newLog += logBuffer[i] + "\n";
                }
            } 
        }

        // Обновляем lastReadIndex до текущего logIndex
        lastReadIndex = logIndex;

        return newLog;
    }

private:
    String logBuffer[LOG_LINES];
    int logIndex = 0;
    bool lastWasNewline = true;

    /**
     * Добавляет сообщение в буфер лога, сохраняя только последние LOG_LINES строк.
     * Если буфер заполнен, старые строки вытесняются (FIFO).
     * 
     * @param msg Сообщение для добавления в буфер.
     * @param newline Указывает, заканчивается ли сообщение новой строкой.
     */
    void addToBuffer(const String& msg, bool newline = true) {
        if (lastWasNewline) {
            // Начинаем новую строку. Добавляем метку времени.
            String logEntry;
            addTimestamp(logEntry);
            logEntry += msg;
            logBuffer[logIndex] = logEntry;
            logIndex = (logIndex + 1) % LOG_LINES; // Перемещаем индекс по принципу FIFO
        } else {
            // Продолжаем предыдущую строку без метки времени.
            logBuffer[(logIndex - 1 + LOG_LINES) % LOG_LINES] += msg;
        }
        lastWasNewline = newline;
    }
};

