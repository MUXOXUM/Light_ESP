// ColorConverter.h - Конвертация температуры и яркости в цвет
#pragma once
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include "config.h"

class ColorConverter {
public:
    // Структура для хранения цветовых компонентов
    struct RGBColor {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        
        // Конструктор по умолчанию (значения по умолчанию из config.h)
        RGBColor() : 
            r(0), g(0), b(0) {}
        
        // Конструктор с параметрами
        RGBColor(uint8_t r, uint8_t g, uint8_t b) : 
            r(r), g(g), b(b) {}
    };

    // Основная функция преобразования: температура + яркость -> RGB
    static RGBColor temperatureAndBrightnessToRGB(int temperature, int brightness) {
        // Проверяем границы
        temperature = constrain(temperature, MIN_TEMPERATURE, MAX_TEMPERATURE);
        brightness = constrain(brightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        
        // 1. Конвертируем температуру в RGB (полная яркость)
        RGBColor baseColor = kelvinToRGB(temperature);
        
        // 2. Применяем яркость
        return applyBrightness(baseColor, brightness);
    }

    // Конвертация температуры Кельвина в RGB
    static RGBColor kelvinToRGB(int kelvin) {
        kelvin = constrain(kelvin, MIN_TEMPERATURE, MAX_TEMPERATURE);
        
        float temp = kelvin / 100.0f;
        float red, green, blue;

        // Формула преобразования (Black Body Radiation)
        if (temp <= 66) {
            red = 255.0f;
            green = temp;
            green = 99.4708025861f * log(green) - 161.1195681661f;
            blue = temp <= 19 ? 0.0f : temp - 10.0f;
            blue = 138.5177312231f * log(blue) - 305.0447927307f;
        } else {
            red = temp - 60.0f;
            red = 329.698727446f * pow(red, -0.1332047592f);
            green = temp - 60.0f;
            green = 288.1221695283f * pow(green, -0.0755148492f);
            blue = 255.0f;
        }

        // Ограничение и округление
        RGBColor result;
        result.r = (uint8_t)constrain(red, 0.0f, 255.0f);
        result.g = (uint8_t)constrain(green, 0.0f, 255.0f);
        result.b = (uint8_t)constrain(blue, 0.0f, 255.0f);

        return result;
    }

    // Применение яркости к цвету
    static RGBColor applyBrightness(const RGBColor& color, int brightnessPercent) {
        brightnessPercent = constrain(brightnessPercent, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        
        float factor = brightnessPercent / 100.0f;
        
        RGBColor result;
        result.r = (uint8_t)(color.r * factor);
        result.g = (uint8_t)(color.g * factor);
        result.b = (uint8_t)(color.b * factor);
        
        return result;
    }

    // Получение цвета для ленты NeoPixel
    static uint32_t toNeoPixelColor(const RGBColor& color) {
        return Adafruit_NeoPixel::Color(color.r, color.g, color.b);
    }

    // Получение цвета по умолчанию
    static RGBColor getDefaultColor() {
        return temperatureAndBrightnessToRGB(DEFAULT_TEMPERATURE, DEFAULT_BRIGHTNESS);
    }

private:
    ColorConverter() = delete; // Статический класс
};