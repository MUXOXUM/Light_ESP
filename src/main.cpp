/*
 * ESP8266 WS2812 Контроллер с веб-интерфейсом
 * Версия 3.5 - Полная конфигурация из одного файла
 */

// Подключение библиотек
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

// Подключение пользовательских файлов
#include "secrets.h"
#include "config.h"
#include "ColorConverter.h"
#include "CurrentLimiter.h"
#include "WebPageBuilder.h"

// Создание объектов
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);

// Глобальные переменные (используем значения по умолчанию из config.h)
int currentBrightness = DEFAULT_BRIGHTNESS;    // Яркость из конфига
int currentTemperature = DEFAULT_TEMPERATURE;  // Температура из конфига
bool isPowerOn = DEFAULT_POWER_ON;             // Состояние питания из конфига

// Адреса в EEPROM для хранения значений
#define EEPROM_SIZE 16
#define EEPROM_ADDR_BRIGHTNESS 0
#define EEPROM_ADDR_TEMPERATURE 2
#define EEPROM_ADDR_POWER 4
#define EEPROM_MAGIC 0xAA55
#define EEPROM_ADDR_MAGIC 10

// Прототипы функций
void setupWiFi();
void setupWebServer();
void handleRoot();
void handleSetParameters();
void handleReset();
void handleGetParameters();
void handlePowerToggle();
void updateLEDs();
void loadFromEEPROM();
void saveToEEPROM();

// ========== SETUP ==========
void setup() {
    // Инициализация Serial порта
    Serial.begin(115200);
    delay(100);
    Serial.println("\n\n=== WS2812 Контроллер Запущен ===");
    
    // Инициализация EEPROM
    EEPROM.begin(EEPROM_SIZE);
    Serial.println("EEPROM инициализирован");
    
    // Загрузка значений из EEPROM
    loadFromEEPROM();
    
    // Инициализация светодиодной ленты
    strip.begin();
    strip.show(); // Очистка ленты
    Serial.println("Лента светодиодов инициализирована");
    
    // Подключение к Wi-Fi
    setupWiFi();
    
    // Настройка веб-сервера
    setupWebServer();
    
    // Установка начальных значений
    updateLEDs();
    
    Serial.println("\nНастройка завершена!");
    Serial.print("Откройте в браузере: http://");
    Serial.println(WiFi.localIP());
}

// ========== LOOP ==========
void loop() {
    server.handleClient();
    yield();
    delay(1);
}

// ========== НАСТРОЙКА WI-FI ==========
void setupWiFi() {
    Serial.print("\nПодключение к Wi-Fi: ");
    Serial.println(WIFI_SSID);
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi подключен!");
    } else {
        Serial.println("\nОшибка подключения к Wi-Fi!");
    }
}

// ========== НАСТРОЙКА ВЕБ-СЕРВЕРА ==========
void setupWebServer() {
    // Маршруты сервера
    server.on("/", handleRoot);
    server.on("/set", HTTP_GET, handleSetParameters);
    server.on("/reset", HTTP_GET, handleReset);
    server.on("/get", HTTP_GET, handleGetParameters);
    server.on("/power", HTTP_GET, handlePowerToggle);
    
    server.begin();
    Serial.println("HTTP сервер запущен на порту 80");
}

// ========== ОБРАБОТЧИК КОРНЕВОЙ СТРАНИЦЫ ==========
void handleRoot() {
    // Генерируем HTML страницу с текущими настройками
    // Используем отдельный блок для освобождения памяти после отправки
    {
        String html = WebPageBuilder::getHTMLPage();
        server.send(200, "text/html", html);
    }
    // Освобождаем память
    yield();
}

// ========== ОБРАБОТЧИК УСТАНОВКИ ПАРАМЕТРОВ ==========
void handleSetParameters() {
    if (server.hasArg("brightness") && server.hasArg("temperature")) {
        int requestedBrightness = server.arg("brightness").toInt();
        int requestedTemperature = server.arg("temperature").toInt();
        
        // Проверяем границы
        requestedBrightness = constrain(requestedBrightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        requestedTemperature = constrain(requestedTemperature, MIN_TEMPERATURE, MAX_TEMPERATURE);
        
        // Применяем ограничение тока к яркости
        currentBrightness = CurrentLimiter::applyCurrentLimit(requestedBrightness, requestedTemperature);
        currentTemperature = requestedTemperature;
        isPowerOn = true;
        
        // Сохраняем в EEPROM
        saveToEEPROM();
        
        // Обновляем светодиоды
        updateLEDs();
        
        // Отправка JSON ответа
        String jsonResponse = "{\"brightness\": " + String(currentBrightness) + 
                             ", \"temperature\": " + String(currentTemperature) + 
                             ", \"power\": " + String(isPowerOn ? 1 : 0) + "}";
        server.send(200, "application/json", jsonResponse);
        yield();
        
    } else {
        server.send(400, "text/plain", "Отсутствуют параметры");
    }
}

// ========== ОБРАБОТЧИК СБРОСА К ЗНАЧЕНИЯМ ПО УМОЛЧАНИЮ ==========
void handleReset() {
    // Сброс к значениям по умолчанию
    currentBrightness = DEFAULT_BRIGHTNESS;
    currentTemperature = DEFAULT_TEMPERATURE;
    isPowerOn = DEFAULT_POWER_ON;
    
    // Сохраняем в EEPROM
    saveToEEPROM();
    
    // Обновляем светодиоды
    updateLEDs();
    
    // Отправка JSON ответа
    String jsonResponse = "{\"brightness\": " + String(currentBrightness) + 
                         ", \"temperature\": " + String(currentTemperature) + 
                         ", \"power\": " + String(isPowerOn ? 1 : 0) + 
                         ", \"message\": \"Сброс к значениям по умолчанию\"}";
    server.send(200, "application/json", jsonResponse);
    yield();
    
}

// ========== ОБРАБОТЧИК ПОЛУЧЕНИЯ ТЕКУЩИХ ПАРАМЕТРОВ ==========
void handleGetParameters() {
    String jsonResponse = "{\"brightness\": " + String(currentBrightness) + 
                         ", \"temperature\": " + String(currentTemperature) + 
                         ", \"power\": " + String(isPowerOn ? 1 : 0) + "}";
    server.send(200, "application/json", jsonResponse);
    yield();
}

// ========== ОБРАБОТЧИК ПЕРЕКЛЮЧЕНИЯ ПИТАНИЯ ==========
void handlePowerToggle() {
    isPowerOn = !isPowerOn;
    
    // Сохраняем в EEPROM
    saveToEEPROM();
    
    // Обновляем светодиоды
    updateLEDs();
    
    // Отправка JSON ответа
    String jsonResponse = "{\"brightness\": " + String(currentBrightness) + 
                         ", \"temperature\": " + String(currentTemperature) + 
                         ", \"power\": " + String(isPowerOn ? 1 : 0) + "}";
    server.send(200, "application/json", jsonResponse);
    yield();
    
}

// ========== ЗАГРУЗКА ИЗ EEPROM ==========
void loadFromEEPROM() {
    // Проверяем магическое число для определения, есть ли сохраненные данные
    uint16_t magic = (EEPROM.read(EEPROM_ADDR_MAGIC) << 8) | EEPROM.read(EEPROM_ADDR_MAGIC + 1);
    
    if (magic == EEPROM_MAGIC) {
        // Читаем сохраненные значения
        currentBrightness = (EEPROM.read(EEPROM_ADDR_BRIGHTNESS) << 8) | EEPROM.read(EEPROM_ADDR_BRIGHTNESS + 1);
        currentTemperature = (EEPROM.read(EEPROM_ADDR_TEMPERATURE) << 8) | EEPROM.read(EEPROM_ADDR_TEMPERATURE + 1);
        isPowerOn = EEPROM.read(EEPROM_ADDR_POWER) != 0;
        
        // Проверяем границы
        currentBrightness = constrain(currentBrightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        currentTemperature = constrain(currentTemperature, MIN_TEMPERATURE, MAX_TEMPERATURE);
        
    } else {
        // Используем значения по умолчанию и сохраняем их
        saveToEEPROM();
    }
}

// ========== СОХРАНЕНИЕ В EEPROM ==========
void saveToEEPROM() {
    // Сохраняем значения
    EEPROM.write(EEPROM_ADDR_BRIGHTNESS, (currentBrightness >> 8) & 0xFF);
    EEPROM.write(EEPROM_ADDR_BRIGHTNESS + 1, currentBrightness & 0xFF);
    
    EEPROM.write(EEPROM_ADDR_TEMPERATURE, (currentTemperature >> 8) & 0xFF);
    EEPROM.write(EEPROM_ADDR_TEMPERATURE + 1, currentTemperature & 0xFF);
    
    EEPROM.write(EEPROM_ADDR_POWER, isPowerOn ? 1 : 0);
    
    // Сохраняем магическое число
    EEPROM.write(EEPROM_ADDR_MAGIC, (EEPROM_MAGIC >> 8) & 0xFF);
    EEPROM.write(EEPROM_ADDR_MAGIC + 1, EEPROM_MAGIC & 0xFF);
    
    // Коммитим изменения
    EEPROM.commit();
    
}

// ========== ОБНОВЛЕНИЕ СВЕТОДИОДОВ ==========
void updateLEDs() {
    if (!isPowerOn || currentBrightness == 0) {
        strip.clear();
        strip.show();
        return;
    }
    
    // Используем ColorConverter для преобразования температуры и яркости в цвет
    ColorConverter::RGBColor color = ColorConverter::temperatureAndBrightnessToRGB(
        currentTemperature, currentBrightness
    );
    
    // Получаем цвет в формате NeoPixel
    uint32_t neoPixelColor = ColorConverter::toNeoPixelColor(color);
    
    // Устанавливаем цвет на все светодиоды
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, neoPixelColor);
    }
    
    strip.show();
}