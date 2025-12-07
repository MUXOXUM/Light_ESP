/*
 * ESP8266 WS2812 Контроллер с веб-интерфейсом
 * Версия 3.5 - Полная конфигурация из одного файла
 */

// Подключение библиотек
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>

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

// Прототипы функций
void setupWiFi();
void setupWebServer();
void handleRoot();
void handleSetParameters();
void handleReset();
void updateLEDs();
void printConfiguration();

// ========== SETUP ==========
void setup() {
    // Инициализация Serial порта
    Serial.begin(115200);
    Serial.println("\n\n=== WS2812 Контроллер Запущен ===");
    
    // Вывод конфигурации
    printConfiguration();
    
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
        Serial.print("IP адрес: ");
        Serial.println(WiFi.localIP());
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
    
    server.begin();
    Serial.println("HTTP сервер запущен на порту 80");
}

// ========== ОБРАБОТЧИК КОРНЕВОЙ СТРАНИЦЫ ==========
void handleRoot() {
    // Генерируем HTML страницу с текущими настройками
    String html = WebPageBuilder::getHTMLPage();
    server.send(200, "text/html", html);
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
        
        // Обновляем светодиоды
        updateLEDs();
        
        // Отправка JSON ответа
        String jsonResponse = "{\"brightness\": " + String(currentBrightness) + 
                             ", \"temperature\": " + String(currentTemperature) + 
                             ", \"power\": " + String(isPowerOn ? 1 : 0) + "}";
        server.send(200, "application/json", jsonResponse);
        
        // Логирование
        Serial.print("Параметры установлены: Яркость=");
        Serial.print(currentBrightness);
        Serial.print("%, Температура=");
        Serial.print(currentTemperature);
        Serial.print("K, Ток=");
        Serial.print(CurrentLimiter::getCurrentConsumption(currentBrightness));
        Serial.println("мА");
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
    
    // Обновляем светодиоды
    updateLEDs();
    
    // Отправка JSON ответа
    String jsonResponse = "{\"brightness\": " + String(currentBrightness) + 
                         ", \"temperature\": " + String(currentTemperature) + 
                         ", \"power\": " + String(isPowerOn ? 1 : 0) + 
                         ", \"message\": \"Сброс к значениям по умолчанию\"}";
    server.send(200, "application/json", jsonResponse);
    
    Serial.print("Сброс к значениям по умолчанию: Яркость=");
    Serial.print(currentBrightness);
    Serial.print("%, Температура=");
    Serial.println(currentTemperature);
}

// ========== ОБНОВЛЕНИЕ СВЕТОДИОДОВ ==========
void updateLEDs() {
    if (!isPowerOn || currentBrightness == 0) {
        strip.clear();
        strip.show();
        Serial.println("Светодиоды выключены");
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
    
    // Логирование для отладки
    Serial.print("Обновление светодиодов: ");
    Serial.print("R=");
    Serial.print(color.r);
    Serial.print(" G=");
    Serial.print(color.g);
    Serial.print(" B=");
    Serial.print(color.b);
    Serial.print(" (");
    Serial.print(currentBrightness);
    Serial.print("%, ");
    Serial.print(currentTemperature);
    Serial.println("K)");
}

// ========== ВЫВОД КОНФИГУРАЦИИ ==========
void printConfiguration() {
    Serial.println("========== КОНФИГУРАЦИЯ ПРОЕКТА ==========");
    Serial.printf("Заголовок: %s\n", HTML_TITLE);
    Serial.printf("Светодиодов: %d\n", NUM_LEDS);
    Serial.printf("Максимальный ток: %d мА\n", MAX_CURRENT_MA);
    Serial.printf("Ток на светодиод: %d мА\n", CURRENT_PER_LED_MA);
    Serial.println();
    Serial.println("ДИАПАЗОНЫ ПАРАМЕТРОВ:");
    Serial.printf("  Яркость: %d-%d%%\n", MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    Serial.printf("  Температура: %d-%dK\n", MIN_TEMPERATURE, MAX_TEMPERATURE);
    Serial.println();
    Serial.println("ЗНАЧЕНИЯ ПО УМОЛЧАНИЮ:");
    Serial.printf("  Яркость: %d%%\n", DEFAULT_BRIGHTNESS);
    Serial.printf("  Температура: %dK\n", DEFAULT_TEMPERATURE);
    Serial.printf("  Питание: %s\n", DEFAULT_POWER_ON ? "ВКЛ" : "ВЫКЛ");
    Serial.println("==========================================");
}