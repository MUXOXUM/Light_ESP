// WebPageBuilder.h - Генератор HTML страницы с подстановкой значений из config.h
#pragma once
#include <Arduino.h>
#include "config.h"

class WebPageBuilder {
public:
    // Получить HTML страницу с подставленными значениями
    static String getHTMLPage() {
        String html = R"=====(
<!DOCTYPE html>
<html lang='ru'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>)=====";
        html += String(HTML_TITLE);
        html += R"=====(</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
        }
        
        body {
            background: #f5f5f5;
            color: #333;
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 500px;
            margin: 0 auto;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
            background: #fff;
            border: 1px solid #e0e0e0;
        }
        
        h1 {
            color: #333;
            font-size: 24px;
            font-weight: 600;
            margin-bottom: 10px;
        }
        
        .status {
            color: #666;
            font-size: 14px;
        }
        
        .control-panel {
            background: #fff;
            border: 1px solid #e0e0e0;
            padding: 20px;
            margin-bottom: 20px;
        }
        
        .power-preview {
            width: 100%;
            height: 120px;
            margin-bottom: 30px;
            border: 2px solid #e0e0e0;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 20px;
            font-weight: 600;
            color: #fff;
            transition: all 0.2s;
            user-select: none;
        }
        
        .power-preview:hover {
            opacity: 0.9;
        }
        
        .power-preview:active {
            opacity: 0.8;
        }
        
        .power-preview.off {
            background: #9e9e9e;
        }
        
        .control-group {
            margin-bottom: 25px;
        }
        
        .control-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 12px;
        }
        
        .control-label {
            font-size: 16px;
            color: #333;
            font-weight: 500;
        }
        
        .value-display {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .value-input {
            width: 80px;
            padding: 8px;
            border: 1px solid #ddd;
            background: #fff;
            color: #333;
            font-size: 16px;
            text-align: center;
        }
        
        .value-input:focus {
            outline: none;
            border-color: #2196F3;
        }
        
        input[type="range"] {
            width: 100%;
            height: 6px;
            -webkit-appearance: none;
            background: #e0e0e0;
            outline: none;
        }
        
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 0;
            background: #2196F3;
            cursor: pointer;
        }
        
        input[type="range"]::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 0;
            background: #2196F3;
            cursor: pointer;
            border: none;
        }
        
        .info-panel {
            background: #fafafa;
            border: 1px solid #e0e0e0;
            padding: 15px;
            margin-top: 20px;
            font-size: 12px;
            text-align: center;
            color: #666;
        }
        
        @media (max-width: 480px) {
            .container {
                padding: 0;
            }
            
            h1 {
                font-size: 20px;
            }
            
            .control-panel {
                padding: 15px;
            }
        }
    </style>
</head>
<body>
    <div class='container'>
        <div class='header'>
            <h1>)=====";
        html += String(HTML_TITLE);
        html += R"=====(</h1>
            <p class='status' id='statusText'>Подключение...</p>
        </div>
        
        <div class='control-panel'>
            <!-- Кнопка питания с предпросмотром цвета -->
            <div class='power-preview' id='powerPreview'>ВЫКЛЮЧЕНО</div>
            
            <!-- Яркость -->
            <div class='control-group'>
                <div class='control-header'>
                    <div class='control-label'>Яркость</div>
                    <div class='value-display'>
                        <input type='number' min=')=====";
        html += String(MIN_BRIGHTNESS);
        html += R"=====(' max=')=====";
        html += String(MAX_BRIGHTNESS);
        html += R"=====(' value=')=====";
        html += String(DEFAULT_BRIGHTNESS);
        html += R"=====(' class='value-input' id='brightnessInput'>
                        <span>%</span>
                    </div>
                </div>
                <input type='range' min=')=====";
        html += String(MIN_BRIGHTNESS);
        html += R"=====(' max=')=====";
        html += String(MAX_BRIGHTNESS);
        html += R"=====(' value=')=====";
        html += String(DEFAULT_BRIGHTNESS);
        html += R"=====(' step='1' class='slider' id='brightnessSlider'>
            </div>
            
            <!-- Температура -->
            <div class='control-group'>
                <div class='control-header'>
                    <div class='control-label'>Температура цвета</div>
                    <div class='value-display'>
                        <input type='number' min=')=====";
        html += String(MIN_TEMPERATURE);
        html += R"=====(' max=')=====";
        html += String(MAX_TEMPERATURE);
        html += R"=====(' value=')=====";
        html += String(DEFAULT_TEMPERATURE);
        html += R"=====(' class='value-input' id='temperatureInput'>
                        <span>K</span>
                    </div>
                </div>
                <input type='range' min=')=====";
        html += String(MIN_TEMPERATURE);
        html += R"=====(' max=')=====";
        html += String(MAX_TEMPERATURE);
        html += R"=====(' value=')=====";
        html += String(DEFAULT_TEMPERATURE);
        html += R"=====(' step='100' class='slider' id='temperatureSlider'>
            </div>
            
            <!-- Информация -->
            <div class='info-panel' id='infoPanel'>
                Макс. ток: )=====";
        html += String(MAX_CURRENT_MA);
        html += R"=====(мА | Светодиодов: )=====";
        html += String(NUM_LEDS);
        html += R"=====(
            </div>
        </div>
    </div>
    
    <script>
        // Текущие значения
        let currentBrightness = )=====";
        html += String(DEFAULT_BRIGHTNESS);
        html += R"=====(;
        let currentTemperature = )=====";
        html += String(DEFAULT_TEMPERATURE);
        html += R"=====(;
        let isPowerOn = )=====";
        html += String(DEFAULT_POWER_ON ? "true" : "false");
        html += R"=====(;
        let deviceIP = '';
        
        // Конфигурационные параметры
        const minBrightness = )=====";
        html += String(MIN_BRIGHTNESS);
        html += R"=====(;
        const maxBrightness = )=====";
        html += String(MAX_BRIGHTNESS);
        html += R"=====(;
        const minTemperature = )=====";
        html += String(MIN_TEMPERATURE);
        html += R"=====(;
        const maxTemperature = )=====";
        html += String(MAX_TEMPERATURE);
        html += R"=====(;
        const defaultBrightness = )=====";
        html += String(DEFAULT_BRIGHTNESS);
        html += R"=====(;
        const defaultTemperature = )=====";
        html += String(DEFAULT_TEMPERATURE);
        html += R"=====(;
        const maxCurrent = )=====";
        html += String(MAX_CURRENT_MA);
        html += R"=====(;
        const numLeds = )=====";
        html += String(NUM_LEDS);
        html += R"=====(;
        
        // Элементы DOM
        const brightnessSlider = document.getElementById('brightnessSlider');
        const brightnessInput = document.getElementById('brightnessInput');
        const temperatureSlider = document.getElementById('temperatureSlider');
        const temperatureInput = document.getElementById('temperatureInput');
        const powerPreview = document.getElementById('powerPreview');
        const statusText = document.getElementById('statusText');
        const infoPanel = document.getElementById('infoPanel');
        
        // Таймер для задержки отправки запросов
        let updateTimeout = null;
        
        // Инициализация
        function init() {
            fetchDeviceIP();
            loadCurrentValues();
            updatePowerPreview();
            updateInfoPanel();
            setupEventListeners();
        }
        
        // Получение IP устройства
        function fetchDeviceIP() {
            const protocol = window.location.protocol;
            const host = window.location.hostname;
            deviceIP = `${protocol}//${host}`;
            statusText.textContent = `Подключено к: ${deviceIP}`;
        }
        
        // Обновление информационной панели
        function updateInfoPanel() {
            infoPanel.textContent = 
                `Макс. ток: ${maxCurrent}мА | ` +
                `Светодиодов: ${numLeds}`;
        }
        
        // Загрузка текущих значений из EEPROM
        async function loadCurrentValues() {
            try {
                const response = await fetch('/get');
                if (response.ok) {
                    const data = await response.json();
                    currentBrightness = data.brightness;
                    currentTemperature = data.temperature;
                    isPowerOn = data.power === 1;
                    
                    // Обновляем UI
                    brightnessSlider.value = currentBrightness;
                    brightnessInput.value = currentBrightness;
                    temperatureSlider.value = currentTemperature;
                    temperatureInput.value = currentTemperature;
                    updatePowerPreview();
                }
            } catch (error) {
                console.error('Ошибка при загрузке значений:', error);
            }
        }
        
        // Обновление предпросмотра питания и цвета
        function updatePowerPreview() {
            if (!isPowerOn) {
                powerPreview.style.background = '#9e9e9e';
                powerPreview.textContent = 'ВЫКЛЮЧЕНО';
                powerPreview.classList.add('off');
            } else {
                powerPreview.classList.remove('off');
                // Рассчитываем цвет на основе температуры и яркости
                const tempRange = maxTemperature - minTemperature;
                const tempRatio = (currentTemperature - minTemperature) / tempRange;
                const hue = 50 - tempRatio * 50; // От желтого (50) к голубому (0)
                const saturation = 80;
                const lightness = Math.min(50, currentBrightness / 2);
                
                powerPreview.style.background = `hsl(${hue}, ${saturation}%, ${lightness}%)`;
                powerPreview.textContent = `ВКЛЮЧЕНО - ${currentTemperature}K, ${currentBrightness}%`;
            }
        }
        
        // Настройка обработчиков событий
        function setupEventListeners() {
            // Кнопка питания
            powerPreview.addEventListener('click', async () => {
                try {
                    const response = await fetch('/power');
                    if (response.ok) {
                        const data = await response.json();
                        isPowerOn = data.power === 1;
                        updatePowerPreview();
                    }
                } catch (error) {
                    console.error('Ошибка при переключении питания:', error);
                }
            });
            
            // Яркость: синхронизация ползунка и поля ввода
            brightnessSlider.addEventListener('input', (e) => {
                const value = parseInt(e.target.value);
                brightnessInput.value = value;
                currentBrightness = value;
                updatePowerPreview();
                scheduleUpdate();
            });
            
            brightnessInput.addEventListener('change', (e) => {
                const value = constrainValue(parseInt(e.target.value), minBrightness, maxBrightness);
                brightnessSlider.value = value;
                brightnessInput.value = value;
                currentBrightness = value;
                updatePowerPreview();
                scheduleUpdate();
            });
            
            // Температура: синхронизация ползунка и поля ввода
            temperatureSlider.addEventListener('input', (e) => {
                const value = parseInt(e.target.value);
                temperatureInput.value = value;
                currentTemperature = value;
                updatePowerPreview();
                scheduleUpdate();
            });
            
            temperatureInput.addEventListener('change', (e) => {
                const value = constrainValue(parseInt(e.target.value), minTemperature, maxTemperature);
                temperatureSlider.value = value;
                temperatureInput.value = value;
                currentTemperature = value;
                updatePowerPreview();
                scheduleUpdate();
            });
            
            // Ограничение ввода при вводе с клавиатуры
            brightnessInput.addEventListener('input', (e) => {
                let value = parseInt(e.target.value) || minBrightness;
                if (value > maxBrightness) {
                    e.target.value = maxBrightness;
                } else if (value < minBrightness) {
                    e.target.value = minBrightness;
                }
            });
            
            temperatureInput.addEventListener('input', (e) => {
                let value = parseInt(e.target.value) || minTemperature;
                if (value > maxTemperature) {
                    e.target.value = maxTemperature;
                } else if (value < minTemperature) {
                    e.target.value = minTemperature;
                }
            });
            
            // Сброс к значениям по умолчанию при двойном клике
            brightnessInput.addEventListener('dblclick', () => {
                brightnessSlider.value = defaultBrightness;
                brightnessInput.value = defaultBrightness;
                currentBrightness = defaultBrightness;
                updatePowerPreview();
                scheduleUpdate();
            });
            
            temperatureInput.addEventListener('dblclick', () => {
                temperatureSlider.value = defaultTemperature;
                temperatureInput.value = defaultTemperature;
                currentTemperature = defaultTemperature;
                updatePowerPreview();
                scheduleUpdate();
            });
        }
        
        // Планирование обновления параметров на устройстве
        function scheduleUpdate() {
            if (updateTimeout) {
                clearTimeout(updateTimeout);
            }
            
            updateTimeout = setTimeout(() => {
                sendUpdateToDevice();
            }, 300);
        }
        
        // Отправка параметров на устройство
        async function sendUpdateToDevice() {
            try {
                const url = `/set?brightness=${currentBrightness}&temperature=${currentTemperature}`;
                const response = await fetch(url);
                
                if (response.ok) {
                    const data = await response.json();
                    
                    // Если сервер скорректировал яркость (ограничение тока)
                    if (data.brightness !== currentBrightness) {
                        currentBrightness = data.brightness;
                        brightnessSlider.value = currentBrightness;
                        brightnessInput.value = currentBrightness;
                        updatePowerPreview();
                    }
                    
                    if (data.temperature !== currentTemperature) {
                        currentTemperature = data.temperature;
                        temperatureSlider.value = currentTemperature;
                        temperatureInput.value = currentTemperature;
                        updatePowerPreview();
                    }
                    
                    isPowerOn = data.power === 1;
                    updatePowerPreview();
                    console.log('Успешно обновлено:', data);
                }
            } catch (error) {
                console.error('Ошибка при обновлении:', error);
            }
        }
        
        // Вспомогательные функции
        function constrainValue(value, min, max) {
            return Math.min(Math.max(value, min), max);
        }
        
        // Инициализация при загрузке
        document.addEventListener('DOMContentLoaded', init);
    </script>
</body>
</html>
)=====";
        
        return html;
    }
};
