// CurrentLimiter.h - Ограничение потребляемого тока
#pragma once
#include "config.h"

class CurrentLimiter {
public:
    // Проверка и коррекция яркости с учетом ограничения тока
    static int applyCurrentLimit(int requestedBrightness, int temperature) {
        // Рассчитываем теоретический ток для запрошенной яркости
        float maxCurrentPerLed = CURRENT_PER_LED_MA;
        float theoreticalCurrent = NUM_LEDS * maxCurrentPerLed * (requestedBrightness / 100.0f);
        
        // Если ток в пределах лимита - возвращаем запрошенную яркость
        if (theoreticalCurrent <= MAX_CURRENT_MA) {
            return requestedBrightness;
        }
        
        // Рассчитываем максимально допустимую яркость
        int safeBrightness = (MAX_CURRENT_MA * 100) / (NUM_LEDS * maxCurrentPerLed);
        safeBrightness = constrain(safeBrightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        
        return safeBrightness;
    }

    // Получение текущего потребления тока
    static float getCurrentConsumption(int brightness) {
        return NUM_LEDS * CURRENT_PER_LED_MA * (brightness / 100.0f);
    }

    // Получение максимального тока для отображения
    static float getMaxCurrent() {
        return MAX_CURRENT_MA;
    }

    // Получение количества светодиодов для отображения
    static int getNumLeds() {
        return NUM_LEDS;
    }

private:
    CurrentLimiter() = delete; // Статический класс
};