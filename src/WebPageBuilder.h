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
            font-family: 'Segoe UI', Arial, sans-serif;
        }
        
        body {
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #fff;
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 600px;
            margin: 0 auto;
            padding: 20px;
        }
        
        .header {
            text-align: center;
            margin-bottom: 40px;
            padding: 20px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.2);
        }
        
        h1 {
            color: #4cc9f0;
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 0 2px 10px rgba(76, 201, 240, 0.3);
        }
        
        .status {
            color: #7eff7a;
            font-size: 0.9em;
            margin-top: 10px;
        }
        
        .control-panel {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 30px;
            margin-bottom: 25px;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }
        
        .control-group {
            margin-bottom: 30px;
        }
        
        .control-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
        }
        
        .control-label {
            font-size: 1.1em;
            color: #f1f1f1;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .control-icon {
            font-size: 1.3em;
        }
        
        .value-display {
            display: flex;
            align-items: center;
            gap: 15px;
        }
        
        .value-input {
            width: 100px;
            padding: 8px 12px;
            border-radius: 8px;
            border: 2px solid #4cc9f0;
            background: rgba(255, 255, 255, 0.1);
            color: white;
            font-size: 1em;
            text-align: center;
        }
        
        .value-input:focus {
            outline: none;
            border-color: #7eff7a;
            box-shadow: 0 0 10px rgba(126, 255, 122, 0.3);
        }
        
        input[type="range"] {
            width: 100%;
            height: 12px;
            -webkit-appearance: none;
            background: linear-gradient(to right, #2d3748, #4a5568);
            border-radius: 10px;
            outline: none;
        }
        
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 28px;
            height: 28px;
            border-radius: 50%;
            background: #4cc9f0;
            cursor: pointer;
            box-shadow: 0 0 15px rgba(76, 201, 240, 0.5);
            transition: all 0.2s;
        }
        
        input[type="range"]::-webkit-slider-thumb:hover {
            transform: scale(1.1);
            box-shadow: 0 0 20px rgba(76, 201, 240, 0.8);
        }
        
        .color-preview {
            width: 100%;
            height: 100px;
            border-radius: 15px;
            margin: 25px 0;
            transition: all 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            font-weight: bold;
            font-size: 1.2em;
            text-shadow: 1px 1px 2px rgba(0,0,0,0.5);
        }
        
        .info-panel {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 10px;
            padding: 15px;
            margin-top: 20px;
            font-size: 0.9em;
            text-align: center;
            color: #a0a0a0;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        @media (max-width: 480px) {
            .container {
                padding: 10px;
            }
            
            h1 {
                font-size: 2em;
            }
            
            .control-panel {
                padding: 20px;
            }
            
            .value-input {
                width: 80px;
            }
        }
    </style>
</head>
<body>
    <div class='container'>
        <div class='header'>
            <h1>🌡️ )=====";
        html += String(HTML_TITLE);
        html += R"=====(</h1>
            <p class='status' id='statusText'>Подключение...</p>
        </div>
        
        <div class='control-panel'>
            <!-- Яркость -->
            <div class='control-group'>
                <div class='control-header'>
                    <div class='control-label'>
                        <span class='control-icon'>💡</span>
                        <span>Яркость</span>
                    </div>
                    <div class='value-display'>
                        <input type='number' min=')=====";
        html += String(MIN_BRIGHTNESS);
        html += R"=====(' max=')=====";
        html += String(MAX_BRIGHTNESS);
        html += R"=====(' value=')=====";
        html += String(DEFAULT_BRIGHTNESS);
        html += R"=====(' 
                               class='value-input' id='brightnessInput'>
                        <span>%</span>
                    </div>
                </div>
                <input type='range' min=')=====";
        html += String(MIN_BRIGHTNESS);
        html += R"=====(' max=')=====";
        html += String(MAX_BRIGHTNESS);
        html += R"=====(' value=')=====";
        html += String(DEFAULT_BRIGHTNESS);
        html += R"=====(' step='1'
                       class='slider' id='brightnessSlider'>
            </div>
            
            <!-- Температура -->
            <div class='control-group'>
                <div class='control-header'>
                    <div class='control-label'>
                        <span class='control-icon'>🌡️</span>
                        <span>Температура цвета</span>
                    </div>
                    <div class='value-display'>
                        <input type='number' min=')=====";
        html += String(MIN_TEMPERATURE);
        html += R"=====(' max=')=====";
        html += String(MAX_TEMPERATURE);
        html += R"=====(' value=')=====";
        html += String(DEFAULT_TEMPERATURE);
        html += R"=====(' 
                               class='value-input' id='temperatureInput'>
                        <span>K</span>
                    </div>
                </div>
                <input type='range' min=')=====";
        html += String(MIN_TEMPERATURE);
        html += R"=====(' max=')=====";
        html += String(MAX_TEMPERATURE);
        html += R"=====(' value=')=====";
        html += String(DEFAULT_TEMPERATURE);
        html += R"=====(' step='100'
                       class='slider' id='temperatureSlider'>
            </div>
            
            <!-- Предпросмотр цвета -->
            <div class='color-preview' id='colorPreview'>
                Цвет предпросмотра
            </div>
            
            <!-- Информация -->
            <div class='info-panel' id='infoPanel'>
                Макс. ток: )=====";
        html += String(MAX_CURRENT_MA);
        html += R"=====(мА | Светодиодов: )=====";
        html += String(NUM_LEDS);
        html += R"=====( | По умолчанию: )=====";
        html += String(DEFAULT_BRIGHTNESS);
        html += R"=====(%, )=====";
        html += String(DEFAULT_TEMPERATURE);
        html += R"=====(K
            </div>
        </div>
    </div>
    
    <script>
        // Текущие значения (берутся из начальных значений в HTML)
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
        
        // Конфигурационные параметры (берутся из HTML атрибутов)
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
        const colorPreview = document.getElementById('colorPreview');
        const statusText = document.getElementById('statusText');
        const infoPanel = document.getElementById('infoPanel');
        
        // Таймер для задержки отправки запросов
        let updateTimeout = null;
        
        // Инициализация
        function init() {
            fetchDeviceIP();
            updateColorPreview();
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
                `Светодиодов: ${numLeds} | ` +
                `По умолчанию: ${defaultBrightness}%, ${defaultTemperature}K`;
        }
        
        // Настройка обработчиков событий
        function setupEventListeners() {
            // Яркость: синхронизация ползунка и поля ввода
            brightnessSlider.addEventListener('input', (e) => {
                const value = parseInt(e.target.value);
                brightnessInput.value = value;
                currentBrightness = value;
                updateColorPreview();
                scheduleUpdate();
            });
            
            brightnessInput.addEventListener('change', (e) => {
                const value = constrainValue(parseInt(e.target.value), minBrightness, maxBrightness);
                brightnessSlider.value = value;
                brightnessInput.value = value;
                currentBrightness = value;
                updateColorPreview();
                scheduleUpdate();
            });
            
            // Температура: синхронизация ползунка и поля ввода
            temperatureSlider.addEventListener('input', (e) => {
                const value = parseInt(e.target.value);
                temperatureInput.value = value;
                currentTemperature = value;
                updateColorPreview();
                scheduleUpdate();
            });
            
            temperatureInput.addEventListener('change', (e) => {
                const value = constrainValue(parseInt(e.target.value), minTemperature, maxTemperature);
                temperatureSlider.value = value;
                temperatureInput.value = value;
                currentTemperature = value;
                updateColorPreview();
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
                updateColorPreview();
                scheduleUpdate();
            });
            
            temperatureInput.addEventListener('dblclick', () => {
                temperatureSlider.value = defaultTemperature;
                temperatureInput.value = defaultTemperature;
                currentTemperature = defaultTemperature;
                updateColorPreview();
                scheduleUpdate();
            });
        }
        
        // Обновление предпросмотра цвета
        function updateColorPreview() {
            // Рассчитываем цвет на основе температуры и яркости
            const tempRange = maxTemperature - minTemperature;
            const tempRatio = (currentTemperature - minTemperature) / tempRange;
            const hue = 50 - tempRatio * 50; // От желтого (50) к голубому (0)
            const saturation = 80;
            const lightness = Math.min(50, currentBrightness / 2);
            
            colorPreview.style.background = `hsl(${hue}, ${saturation}%, ${lightness}%)`;
            colorPreview.textContent = `${currentTemperature}K, ${currentBrightness}%`;
            colorPreview.style.color = lightness > 30 ? '#000' : '#fff';
        }
        
        // Планирование обновления параметров на устройстве
        function scheduleUpdate() {
            if (updateTimeout) {
                clearTimeout(updateTimeout);
            }
            
            updateTimeout = setTimeout(() => {
                sendUpdateToDevice();
            }, 300); // Задержка 300 мс
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
                        updateColorPreview();
                        showNotification('Яркость скорректирована системой ограничения тока');
                    }
                    
                    if (data.temperature !== currentTemperature) {
                        currentTemperature = data.temperature;
                        temperatureSlider.value = currentTemperature;
                        temperatureInput.value = currentTemperature;
                        updateColorPreview();
                    }
                    
                    isPowerOn = data.power === 1;
                    console.log('Успешно обновлено:', data);
                }
            } catch (error) {
                console.error('Ошибка при обновлении:', error);
                showNotification('Ошибка подключения к устройству');
            }
        }
        
        // Вспомогательные функции
        function constrainValue(value, min, max) {
            return Math.min(Math.max(value, min), max);
        }
        
        function showNotification(message) {
            console.log('Уведомление:', message);
            if ('Notification' in window && Notification.permission === 'granted') {
                new Notification(message);
            }
        }
        
        // Функция сброса к значениям по умолчанию
        function resetToDefaults() {
            currentBrightness = defaultBrightness;
            currentTemperature = defaultTemperature;
            
            brightnessSlider.value = currentBrightness;
            brightnessInput.value = currentBrightness;
            temperatureSlider.value = currentTemperature;
            temperatureInput.value = currentTemperature;
            
            updateColorPreview();
            scheduleUpdate();
        }
        
        // Горячие клавиши
        document.addEventListener('keydown', (e) => {
            // Ctrl+R - сброс к значениям по умолчанию
            if (e.ctrlKey && e.key === 'r') {
                e.preventDefault();
                resetToDefaults();
                showNotification('Сброс к значениям по умолчанию');
            }
        });
        
        // Инициализация при загрузке
        document.addEventListener('DOMContentLoaded', init);
    </script>
</body>
</html>
)=====";
        
        return html;
    }
};