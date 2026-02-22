#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <time.h>

#include "secrets.h"

namespace {
constexpr uint8_t LED_PIN = D4;        // GPIO2
constexpr uint8_t BTN_PIN = D5;
constexpr uint16_t LED_COUNT = 30;
constexpr uint16_t EEPROM_SIZE = 64;
constexpr uint32_t SAVE_DELAY_MS = 1200;
constexpr uint32_t WIFI_RETRY_MS = 5000;
constexpr uint16_t KELVIN_MIN = 1000;
constexpr uint16_t KELVIN_MAX = 4000;
constexpr int32_t TZ_OFFSET_SECONDS = 3 * 3600;
constexpr uint32_t SCHEDULE_CHECK_MS = 2000;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;

struct PersistedSettings {
  uint8_t marker;
  uint8_t brightness;
  uint16_t temperature;
  uint8_t power;
  uint8_t onHour;
  uint8_t onMinute;
  uint8_t offHour;
  uint8_t offMinute;
  uint8_t scheduleEnabled;
};

PersistedSettings settings = {0xA5, 180, 2000, 1, 18, 0, 23, 30, 0};

ESP8266WebServer server(80);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

bool pendingSave = false;
uint32_t saveRequestedAt = 0;
uint32_t lastWifiRetryAt = 0;
uint32_t lastScheduleCheckAt = 0;

bool buttonStableState = true;
bool buttonLastReading = true;
uint32_t buttonLastChangeAt = 0;

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="ru">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP8266 Light Control</title>
  <style>
    :root {
      --bg1: #10131a;
      --bg2: #1b2230;
      --card: #ffffff;
      --txt: #111218;
      --accent: #2f80ed;
      --muted: #6c7485;
    }
    body {
      margin: 0;
      min-height: 100vh;
      display: grid;
      place-items: center;
      font-family: "Segoe UI", "Arial", sans-serif;
      background: radial-gradient(circle at 20% 10%, #344059 0%, var(--bg1) 45%, #0b0d11 100%);
      color: var(--txt);
    }
    .card {
      width: min(92vw, 460px);
      background: var(--card);
      border-radius: 16px;
      padding: 22px;
      box-shadow: 0 18px 50px rgba(0, 0, 0, 0.35);
    }
    h1 {
      margin: 0 0 14px;
      font-size: 1.25rem;
    }
    .row {
      margin: 14px 0;
    }
    .label {
      display: flex;
      justify-content: space-between;
      font-size: 0.93rem;
      color: var(--muted);
      margin-bottom: 6px;
    }
    input[type="range"] {
      width: 100%;
      accent-color: var(--accent);
    }
    button {
      width: 100%;
      border: 0;
      border-radius: 10px;
      padding: 12px;
      font-size: 1rem;
      cursor: pointer;
      color: #fff;
      background: linear-gradient(135deg, #2f80ed, #2d9ee0);
    }
    button.off {
      background: linear-gradient(135deg, #6b7280, #4b5563);
    }
    .status {
      margin-top: 10px;
      font-size: 0.86rem;
      color: var(--muted);
      text-align: center;
    }
  </style>
</head>
<body>
  <main class="card">
    <h1>WS2812 Controller</h1>

    <div class="row">
      <div class="label"><span>Яркость</span><span id="brightnessValue">0</span></div>
      <input id="brightness" type="range" min="0" max="255" step="1" />
    </div>

    <div class="row">
      <div class="label"><span>Температура</span><span id="temperatureValue">0 K</span></div>
      <input id="temperature" type="range" min="2700" max="6500" step="10" />
    </div>

    <div class="row">
      <button id="toggleBtn">Включено</button>
    </div>

    <div class="row">
      <div class="label"><span>Расписание</span><span id="scheduleValue">выключено</span></div>
      <label style="display:flex;gap:10px;align-items:center;font-size:0.95rem;color:var(--muted);">
        <input id="scheduleEnabled" type="checkbox" />
        Использовать расписание
      </label>
    </div>

    <div class="row" style="display:grid;gap:10px;">
      <div class="label"><span>Время включения (UTC+3)</span><span id="onTimeValue">--:--</span></div>
      <input id="onTime" type="time" />
      <div class="label"><span>Время выключения (UTC+3)</span><span id="offTimeValue">--:--</span></div>
      <input id="offTime" type="time" />
    </div>

    <div class="status" id="status">Синхронизация...</div>
  </main>

  <script>
    const brightness = document.getElementById('brightness');
    const temperature = document.getElementById('temperature');
    const brightnessValue = document.getElementById('brightnessValue');
    const temperatureValue = document.getElementById('temperatureValue');
    const toggleBtn = document.getElementById('toggleBtn');
    const statusEl = document.getElementById('status');
    const scheduleEnabled = document.getElementById('scheduleEnabled');
    const scheduleValue = document.getElementById('scheduleValue');
    const onTime = document.getElementById('onTime');
    const offTime = document.getElementById('offTime');
    const onTimeValue = document.getElementById('onTimeValue');
    const offTimeValue = document.getElementById('offTimeValue');

    let state = {
      brightness: 180,
      temperature: 4000,
      on: true,
      scheduleEnabled: false,
      onTime: '18:00',
      offTime: '23:30'
    };
    let timer = null;

    function render() {
      brightness.value = state.brightness;
      temperature.value = state.temperature;
      brightnessValue.textContent = state.brightness;
      temperatureValue.textContent = `${state.temperature} K`;
      toggleBtn.textContent = state.on ? 'Включено' : 'Выключено';
      toggleBtn.classList.toggle('off', !state.on);
      scheduleEnabled.checked = state.scheduleEnabled;
      scheduleValue.textContent = state.scheduleEnabled ? 'активно' : 'выключено';
      onTime.value = state.onTime;
      offTime.value = state.offTime;
      onTimeValue.textContent = state.onTime;
      offTimeValue.textContent = state.offTime;
    }

    async function pushState() {
      const query = new URLSearchParams({
        brightness: state.brightness,
        temperature: state.temperature,
        on: state.on ? '1' : '0',
        schedule: state.scheduleEnabled ? '1' : '0',
        onTime: state.onTime,
        offTime: state.offTime
      });

      const response = await fetch(`/api/set?${query.toString()}`);
      const data = await response.json();
      state = {
        brightness: data.brightness,
        temperature: data.temperature,
        on: Boolean(data.on),
        scheduleEnabled: Boolean(data.schedule),
        onTime: data.onTime,
        offTime: data.offTime
      };
      statusEl.textContent = `IP: ${data.ip} | Wi-Fi: ${data.wifi} | Время: ${data.time}`;
      render();
    }

    function schedulePush() {
      if (timer) clearTimeout(timer);
      timer = setTimeout(() => {
        pushState().catch(() => statusEl.textContent = 'Ошибка связи с ESP8266');
      }, 80);
    }

    brightness.addEventListener('input', () => {
      state.brightness = Number(brightness.value);
      brightnessValue.textContent = state.brightness;
      schedulePush();
    });

    temperature.addEventListener('input', () => {
      state.temperature = Number(temperature.value);
      temperatureValue.textContent = `${state.temperature} K`;
      schedulePush();
    });

    toggleBtn.addEventListener('click', () => {
      state.on = !state.on;
      render();
      schedulePush();
    });

    scheduleEnabled.addEventListener('change', () => {
      state.scheduleEnabled = scheduleEnabled.checked;
      render();
      schedulePush();
    });

    onTime.addEventListener('change', () => {
      state.onTime = onTime.value || state.onTime;
      render();
      schedulePush();
    });

    offTime.addEventListener('change', () => {
      state.offTime = offTime.value || state.offTime;
      render();
      schedulePush();
    });

    async function loadInitialState() {
      const response = await fetch('/api/state');
      const data = await response.json();
      state = {
        brightness: data.brightness,
        temperature: data.temperature,
        on: Boolean(data.on),
        scheduleEnabled: Boolean(data.schedule),
        onTime: data.onTime,
        offTime: data.offTime
      };
      statusEl.textContent = `IP: ${data.ip} | Wi-Fi: ${data.wifi} | Время: ${data.time}`;
      render();
    }

    loadInitialState().catch(() => {
      statusEl.textContent = 'Не удалось получить состояние';
      render();
    });
  </script>
</body>
</html>
)HTML";

uint8_t clampU8(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > 255) {
    return 255;
  }
  return static_cast<uint8_t>(value);
}

uint8_t applyBrightness(uint8_t channel, uint8_t brightness) {
  return static_cast<uint8_t>((static_cast<uint16_t>(channel) * brightness) / 255);
}

void temperatureToRGB(uint16_t kelvin, uint8_t &r, uint8_t &g, uint8_t &b) {
  float temp = static_cast<float>(kelvin) / 100.0f;

  float red;
  float green;
  float blue;

  if (temp <= 66.0f) {
    red = 255.0f;
  } else {
    red = 329.698727446f * powf(temp - 60.0f, -0.1332047592f);
  }

  if (temp <= 66.0f) {
    green = 99.4708025861f * logf(temp) - 161.1195681661f;
  } else {
    green = 288.1221695283f * powf(temp - 60.0f, -0.0755148492f);
  }

  if (temp >= 66.0f) {
    blue = 255.0f;
  } else if (temp <= 19.0f) {
    blue = 0.0f;
  } else {
    blue = 138.5177312231f * logf(temp - 10.0f) - 305.0447927307f;
  }

  r = clampU8(static_cast<int>(red));
  g = clampU8(static_cast<int>(green));
  b = clampU8(static_cast<int>(blue));
}

void applyStripState() {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (settings.power != 0) {
    temperatureToRGB(settings.temperature, r, g, b);
    r = applyBrightness(r, settings.brightness);
    g = applyBrightness(g, settings.brightness);
    b = applyBrightness(b, settings.brightness);
  }

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }

  strip.show();

  Serial.printf("[LED] Power=%u Brightness=%u Temp=%uK RGB=(%u,%u,%u)\n",
                settings.power,
                settings.brightness,
                settings.temperature,
                r,
                g,
                b);
}

void requestSave() {
  pendingSave = true;
  saveRequestedAt = millis();
}

void saveSettingsIfNeeded() {
  if (!pendingSave || millis() - saveRequestedAt < SAVE_DELAY_MS) {
    return;
  }

  EEPROM.put(0, settings);
  bool committed = EEPROM.commit();
  pendingSave = false;

  Serial.printf("[EEPROM] Save %s (brightness=%u, temp=%u, power=%u)\n",
                committed ? "OK" : "FAILED",
                settings.brightness,
                settings.temperature,
                settings.power);
}

void loadSettings() {
  PersistedSettings loaded{};
  EEPROM.get(0, loaded);

  bool isValid = loaded.marker == 0xA5 &&
                 loaded.temperature >= KELVIN_MIN && loaded.temperature <= KELVIN_MAX &&
                 loaded.power <= 1 &&
                 loaded.onHour < 24 && loaded.offHour < 24 &&
                 loaded.onMinute < 60 && loaded.offMinute < 60 &&
                 loaded.scheduleEnabled <= 1;

  if (isValid) {
    settings = loaded;
    Serial.printf("[EEPROM] Loaded: brightness=%u temp=%u power=%u\n",
                  settings.brightness,
                  settings.temperature,
                  settings.power);
  } else {
    Serial.println("[EEPROM] No valid saved settings, using defaults");
    requestSave();
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250);
    Serial.print('.');
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[WiFi] Initial connection failed, retry will continue in loop()");
  }
}

void initTimeSync() {
  configTime(TZ_OFFSET_SECONDS, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  Serial.println("[TIME] NTP sync configured (UTC+3)");
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (millis() - lastWifiRetryAt < WIFI_RETRY_MS) {
    return;
  }

  lastWifiRetryAt = millis();
  Serial.println("[WiFi] Disconnected, trying reconnect...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void initStrip() {
  strip.begin();
  strip.setBrightness(255);
  strip.show();
  applyStripState();
}

String getWifiStatusText() {
  return WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
}

bool getLocalTime(struct tm &info) {
  time_t now = time(nullptr);
  if (now < 1600000000) {
    return false;
  }
  localtime_r(&now, &info);
  return true;
}

String formatTwoDigits(int value) {
  if (value < 10) {
    return "0" + String(value);
  }
  return String(value);
}

String formatTime(uint8_t hour, uint8_t minute) {
  return formatTwoDigits(hour) + ":" + formatTwoDigits(minute);
}

bool isTimeInRange(uint8_t hour, uint8_t minute, uint8_t onHour, uint8_t onMinute, uint8_t offHour, uint8_t offMinute) {
  int current = hour * 60 + minute;
  int on = onHour * 60 + onMinute;
  int off = offHour * 60 + offMinute;

  if (on == off) {
    return false;
  }

  if (on < off) {
    return current >= on && current < off;
  }

  return current >= on || current < off;
}

void applyScheduleIfNeeded() {
  if (!settings.scheduleEnabled) {
    return;
  }

  if (millis() - lastScheduleCheckAt < SCHEDULE_CHECK_MS) {
    return;
  }
  lastScheduleCheckAt = millis();

  struct tm now{};
  if (!getLocalTime(now)) {
    return;
  }

  bool shouldBeOn = isTimeInRange(
      static_cast<uint8_t>(now.tm_hour),
      static_cast<uint8_t>(now.tm_min),
      settings.onHour,
      settings.onMinute,
      settings.offHour,
      settings.offMinute);

  uint8_t target = shouldBeOn ? 1 : 0;
  if (settings.power != target) {
    settings.power = target;
    applyStripState();
  }
}

void handleButton() {
  bool reading = digitalRead(BTN_PIN) == HIGH;
  if (reading != buttonLastReading) {
    buttonLastChangeAt = millis();
  }

  if (millis() - buttonLastChangeAt > BUTTON_DEBOUNCE_MS && reading != buttonStableState) {
    buttonStableState = reading;
    if (!buttonStableState) {
      settings.power = settings.power ? 0 : 1;
      applyStripState();
      requestSave();
    }
  }

  buttonLastReading = reading;
}

void sendStateJson() {
  struct tm now{};
  String timeText = getLocalTime(now) ? (formatTwoDigits(now.tm_hour) + ":" + formatTwoDigits(now.tm_min)) : "--:--";

  String json = "{";
  json += "\"brightness\":" + String(settings.brightness);
  json += ",\"temperature\":" + String(settings.temperature);
  json += ",\"on\":" + String(settings.power);
  json += ",\"schedule\":" + String(settings.scheduleEnabled);
  json += ",\"onTime\":\"" + formatTime(settings.onHour, settings.onMinute) + "\"";
  json += ",\"offTime\":\"" + formatTime(settings.offHour, settings.offMinute) + "\"";
  json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"wifi\":\"" + getWifiStatusText() + "\"";
  json += ",\"time\":\"" + timeText + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleState() {
  sendStateJson();
}

void handleSet() {
  if (server.hasArg("brightness")) {
    int value = server.arg("brightness").toInt();
    settings.brightness = clampU8(value);
  }

  if (server.hasArg("temperature")) {
    int value = server.arg("temperature").toInt();
    if (value < KELVIN_MIN) {
      value = KELVIN_MIN;
    }
    if (value > KELVIN_MAX) {
      value = KELVIN_MAX;
    }
    settings.temperature = static_cast<uint16_t>(value);
  }

  if (server.hasArg("on")) {
    settings.power = server.arg("on").toInt() != 0 ? 1 : 0;
  }

  if (server.hasArg("schedule")) {
    settings.scheduleEnabled = server.arg("schedule").toInt() != 0 ? 1 : 0;
  }

  if (server.hasArg("onTime")) {
    String timeStr = server.arg("onTime");
    int sep = timeStr.indexOf(':');
    if (sep > 0) {
      int h = timeStr.substring(0, sep).toInt();
      int m = timeStr.substring(sep + 1).toInt();
      if (h >= 0 && h < 24 && m >= 0 && m < 60) {
        settings.onHour = static_cast<uint8_t>(h);
        settings.onMinute = static_cast<uint8_t>(m);
      }
    }
  }

  if (server.hasArg("offTime")) {
    String timeStr = server.arg("offTime");
    int sep = timeStr.indexOf(':');
    if (sep > 0) {
      int h = timeStr.substring(0, sep).toInt();
      int m = timeStr.substring(sep + 1).toInt();
      if (h >= 0 && h < 24 && m >= 0 && m < 60) {
        settings.offHour = static_cast<uint8_t>(h);
        settings.offMinute = static_cast<uint8_t>(m);
      }
    }
  }

  applyStripState();
  requestSave();
  sendStateJson();
}

void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/set", HTTP_GET, handleSet);

  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"not_found\"}");
  });

  server.begin();
  Serial.println("[HTTP] Server started on port 80");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("[SYS] Booting...");

  EEPROM.begin(EEPROM_SIZE);
  loadSettings();

  pinMode(BTN_PIN, INPUT_PULLUP);

  initStrip();
  connectWiFi();
  initTimeSync();
  setupServer();
}

void loop() {
  server.handleClient();
  maintainWiFi();
  handleButton();
  applyScheduleIfNeeded();
  saveSettingsIfNeeded();
}
