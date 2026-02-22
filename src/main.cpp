#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <time.h>

#include "secrets.h"

namespace {
constexpr uint8_t LED_PIN = D4;
constexpr uint8_t BTN_PIN = D5;
constexpr uint16_t LED_COUNT = 63;
constexpr uint16_t EEPROM_SIZE = 64;
constexpr uint32_t SAVE_DELAY_MS = 1200;
constexpr uint32_t WIFI_RETRY_MS = 5000;
constexpr uint16_t KELVIN_MIN = 1000;
constexpr uint16_t KELVIN_MAX = 4000;
constexpr int8_t TIMEZONE_UTC_HOURS = 3;
constexpr int32_t TZ_OFFSET_SECONDS = TIMEZONE_UTC_HOURS * 3600;
constexpr uint16_t MAX_STRIP_CURRENT_MA = 1800;
constexpr uint32_t SCHEDULE_CHECK_MS = 2000;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 60;
constexpr uint32_t POWER_FADE_DURATION_MS = 1000;
constexpr uint32_t FADE_FRAME_INTERVAL_MS = 16;

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

PersistedSettings settings = {0xA5, 70, 2000, 1, 18, 0, 23, 30, 0};

ESP8266WebServer server(80);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

bool pendingSave = false;
uint32_t saveRequestedAt = 0;
uint32_t lastWifiRetryAt = 0;
uint32_t lastScheduleCheckAt = 0;
uint32_t fadeStartedAt = 0;
uint32_t lastFadeFrameAt = 0;

bool buttonStableState = true;
bool buttonLastReading = true;
uint32_t buttonLastChangeAt = 0;
bool fadeActive = false;
uint8_t fadeStartScale255 = 0;
uint8_t fadeTargetScale255 = 0;
uint8_t lastPowerState = 0;

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
    .collapsible {
      display: grid;
      gap: 10px;
      overflow: hidden;
      max-height: 220px;
      opacity: 1;
      transform: translateY(0);
      transition: max-height 380ms ease, opacity 320ms ease, transform 380ms ease, margin 380ms ease;
    }
    .collapsible.collapsed {
      max-height: 0;
      opacity: 0;
      transform: translateY(-6px);
      margin: 0;
      pointer-events: none;
    }
    .toggle-collapsible {
      overflow: hidden;
      max-height: 64px;
      opacity: 1;
      transform: translateY(0);
      transition: max-height 300ms ease, opacity 240ms ease, transform 300ms ease, margin 300ms ease;
    }
    .toggle-collapsible.collapsed {
      max-height: 0;
      opacity: 0;
      transform: translateY(-6px);
      margin: 0;
      pointer-events: none;
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
      <div class="label"><span>Яркость</span><span id="brightnessValue">0%</span></div>
      <input id="brightness" type="range" min="0" max="100" step="1" />
    </div>

    <div class="row">
      <div class="label"><span>Температура</span><span id="temperatureValue">0 K</span></div>
      <input id="temperature" type="range" min="1000" max="4000" step="10" />
    </div>

    <div id="toggleRow" class="row toggle-collapsible">
      <button id="toggleBtn">Включено</button>
    </div>

    <div class="row">
      <div class="label"><span>Расписание</span><span id="scheduleValue">выключено</span></div>
      <label style="display:flex;gap:10px;align-items:center;font-size:0.95rem;color:var(--muted);">
        <input id="scheduleEnabled" type="checkbox" />
        Использовать расписание
      </label>
    </div>

    <div id="scheduleSettingsRow" class="row collapsible collapsed">
      <div class="label"><span>Время включения (локальное)</span><span id="onTimeValue">--:--</span></div>
      <input id="onTime" type="time" />
      <div class="label"><span>Время выключения (локальное)</span><span id="offTimeValue">--:--</span></div>
      <input id="offTime" type="time" />
    </div>

    <div class="status" id="status">Синхронизация...</div>
  </main>

  <script>
    const brightness = document.getElementById('brightness');
    const temperature = document.getElementById('temperature');
    const brightnessValue = document.getElementById('brightnessValue');
    const temperatureValue = document.getElementById('temperatureValue');
    const toggleRow = document.getElementById('toggleRow');
    const toggleBtn = document.getElementById('toggleBtn');
    const statusEl = document.getElementById('status');
    const scheduleEnabled = document.getElementById('scheduleEnabled');
    const scheduleValue = document.getElementById('scheduleValue');
    const scheduleSettingsRow = document.getElementById('scheduleSettingsRow');
    const onTime = document.getElementById('onTime');
    const offTime = document.getElementById('offTime');
    const onTimeValue = document.getElementById('onTimeValue');
    const offTimeValue = document.getElementById('offTimeValue');

    let state = {
      brightness: 70,
      temperature: 2000,
      on: true,
      scheduleEnabled: false,
      onTime: '18:00',
      offTime: '23:30'
    };
    let timer = null;
    let pushSeq = 0;
    let syncTimer = null;

    function render() {
      brightness.value = state.brightness;
      temperature.value = state.temperature;
      brightnessValue.textContent = `${state.brightness}%`;
      temperatureValue.textContent = `${state.temperature} K`;
      toggleBtn.textContent = state.on ? 'Включено' : 'Выключено';
      toggleBtn.classList.toggle('off', !state.on);
      toggleRow.classList.toggle('collapsed', state.scheduleEnabled);
      scheduleEnabled.checked = state.scheduleEnabled;
      scheduleValue.textContent = state.scheduleEnabled ? 'активно' : 'выключено';
      scheduleSettingsRow.classList.toggle('collapsed', !state.scheduleEnabled);
      onTime.value = state.onTime;
      offTime.value = state.offTime;
      onTimeValue.textContent = state.onTime;
      offTimeValue.textContent = state.offTime;
    }

    async function pushState() {
      const seq = ++pushSeq;
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
      if (seq !== pushSeq) {
        return;
      }
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

    async function syncStateSilently() {
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

    function schedulePush() {
      if (timer) clearTimeout(timer);
      timer = setTimeout(() => {
        pushState().catch(() => statusEl.textContent = 'Ошибка связи с ESP8266');
      }, 80);
    }

    brightness.addEventListener('input', () => {
      state.brightness = Number(brightness.value);
      brightnessValue.textContent = `${state.brightness}%`;
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
      await syncStateSilently();
      if (syncTimer) {
        clearInterval(syncTimer);
      }
      syncTimer = setInterval(() => {
        syncStateSilently().catch(() => {});
      }, 2000);
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
  return static_cast<uint8_t>((static_cast<uint16_t>(channel) * brightness) / 100);
}

uint8_t scaleChannelBy255(uint8_t channel, uint8_t scale255) {
  return static_cast<uint8_t>((static_cast<uint16_t>(channel) * scale255) / 255);
}

uint32_t estimateCurrentMa(uint8_t r, uint8_t g, uint8_t b) {
  // Approximation for WS2812: up to 20mA per color channel at value 255.
  uint32_t rgbSum = static_cast<uint32_t>(r) + g + b;
  uint64_t numerator = static_cast<uint64_t>(LED_COUNT) * rgbSum * 20;
  return static_cast<uint32_t>((numerator + 254) / 255);
}

void limitRgbByCurrent(uint8_t &r, uint8_t &g, uint8_t &b) {
  uint32_t estimatedCurrentMa = estimateCurrentMa(r, g, b);
  if (estimatedCurrentMa <= MAX_STRIP_CURRENT_MA || estimatedCurrentMa == 0) {
    return;
  }

  uint32_t scale = (static_cast<uint32_t>(MAX_STRIP_CURRENT_MA) * 255) / estimatedCurrentMa;
  if (scale > 255) {
    scale = 255;
  }

  uint8_t scale255 = static_cast<uint8_t>(scale);
  r = scaleChannelBy255(r, scale255);
  g = scaleChannelBy255(g, scale255);
  b = scaleChannelBy255(b, scale255);
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

uint8_t getCurrentFadeScale255() {
  if (!fadeActive) {
    return fadeTargetScale255;
  }

  uint32_t elapsed = millis() - fadeStartedAt;
  if (elapsed >= POWER_FADE_DURATION_MS) {
    fadeActive = false;
    return fadeTargetScale255;
  }

  int32_t delta = static_cast<int32_t>(fadeTargetScale255) - static_cast<int32_t>(fadeStartScale255);
  int32_t value = static_cast<int32_t>(fadeStartScale255) + (delta * static_cast<int32_t>(elapsed)) / static_cast<int32_t>(POWER_FADE_DURATION_MS);
  return clampU8(value);
}

void startPowerFade(uint8_t targetScale255) {
  uint8_t currentScale255 = getCurrentFadeScale255();
  fadeStartScale255 = currentScale255;
  fadeTargetScale255 = targetScale255;
  fadeStartedAt = millis();
  fadeActive = fadeStartScale255 != fadeTargetScale255;
}

void applyStripState(bool logState = true) {
  if (settings.power != lastPowerState) {
    lastPowerState = settings.power;
    startPowerFade(settings.power != 0 ? 255 : 0);
  }

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t powerScale255 = getCurrentFadeScale255();

  if (powerScale255 > 0) {
    temperatureToRGB(settings.temperature, r, g, b);
    r = applyBrightness(r, settings.brightness);
    g = applyBrightness(g, settings.brightness);
    b = applyBrightness(b, settings.brightness);
    limitRgbByCurrent(r, g, b);
    r = scaleChannelBy255(r, powerScale255);
    g = scaleChannelBy255(g, powerScale255);
    b = scaleChannelBy255(b, powerScale255);
  }

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }

  strip.show();

  if (logState) {
    Serial.printf("[LED] Power=%u Brightness=%u Temp=%uK Fade=%u RGB=(%u,%u,%u) MaxCurrent=%umA\n",
                  settings.power,
                  settings.brightness,
                  settings.temperature,
                  powerScale255,
                  r,
                  g,
                  b,
                  MAX_STRIP_CURRENT_MA);
  }
}

void updateFadeAnimation() {
  if (!fadeActive) {
    return;
  }

  if (millis() - lastFadeFrameAt < FADE_FRAME_INTERVAL_MS) {
    return;
  }
  lastFadeFrameAt = millis();
  applyStripState(false);
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
    // Migrate legacy brightness scale 0..255 to percent scale 0..100.
    if (loaded.brightness > 100) {
      loaded.brightness = static_cast<uint8_t>((static_cast<uint16_t>(loaded.brightness) * 100 + 127) / 255);
    }
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
  Serial.printf("[TIME] NTP sync configured (UTC%+d)\n", TIMEZONE_UTC_HOURS);
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
    if (value < 0) {
      value = 0;
    }
    if (value > 100) {
      value = 100;
    }
    settings.brightness = static_cast<uint8_t>(value);
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

  // Sync fade state with loaded power state to avoid false transition on boot.
  lastPowerState = settings.power;
  fadeStartScale255 = settings.power != 0 ? 255 : 0;
  fadeTargetScale255 = fadeStartScale255;
  fadeActive = false;

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
  updateFadeAnimation();
  applyScheduleIfNeeded();
  saveSettingsIfNeeded();
}
