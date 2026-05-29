// AirQualityWeMosSensors
// WeMos D1 Mini — PMS5003 + BME680 sensor node
//
// Required libraries (install via Arduino Library Manager):
//   - WiFiManager          by tzapu            >= 2.0
//   - Adafruit BME680      by Adafruit          >= 2.0
//   - Adafruit BusIO       by Adafruit          (dependency)
//   - ArduinoJson          by Benoit Blanchon   >= 7.0
//   - PMS                  by Mariusz Kacki     >= 1.1
//
// Board: LOLIN(WEMOS) D1 R2 & mini  (esp8266 by ESP8266 Community package)

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

#include "config.h"
#include "LedController.h"
#include "SensorManager.h"

SensorManager sensors;

// ── WiFi reconnection helper ──────────────────────────────────────────────────
static bool ensureWifi() {
    if (WiFi.status() == WL_CONNECTED) return true;

    Serial.println(F("[WiFi] Connection lost — attempting reconnect..."));
    WiFi.reconnect();

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > 15000UL) {
            Serial.println(F("[WiFi] Reconnect timed out."));
            return false;
        }
        delay(500);
        Serial.print(F("."));
    }
    Serial.printf("\n[WiFi] Reconnected — IP: %s\n",
                  WiFi.localIP().toString().c_str());
    return true;
}

// ── HTTP POST ─────────────────────────────────────────────────────────────────
static void postReadings(const PmsData& pms, const BmeData& bme) {
    if (!pms.valid && !bme.valid) {
        Serial.println(F("[HTTP] Both reads invalid — skipping POST."));
        return;
    }

    if (!ensureWifi()) return;

    JsonDocument doc;
    if (pms.valid) {
        doc["pm1_0"] = pms.pm1_0;
        doc["pm2_5"] = pms.pm2_5;
        doc["pm10"]  = pms.pm10;
    }
    if (bme.valid) {
        doc["temperature"] = bme.temperature;
        doc["humidity"]    = bme.humidity;
        doc["pressure"]    = bme.pressure;
        doc["altitude"]    = bme.altitude;
    }

    String payload;
    serializeJson(doc, payload);

    String url = String(F("http://")) + RECEIVER_HOST + F(":") +
                 RECEIVER_PORT + RECEIVER_PATH;

    Serial.println(String(F("[HTTP] POST ")) + url);
    Serial.println(String(F("[HTTP] ")) + payload);

    ledWifiTx();

    WiFiClient client;
    HTTPClient  http;
    http.begin(client, url);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader(F("Content-Type"), F("application/json"));

    int code = http.POST(payload);
    http.end();

    if (code == HTTP_CODE_CREATED || code == HTTP_CODE_OK) {
        Serial.printf("[HTTP] Success (%d)\n", code);
    } else {
        Serial.printf("[HTTP] Failed — status %d\n", code);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println(F("\n[Boot] AirQualityWeMos starting..."));

    ledBegin();

    // WiFiManager: on first boot creates "AirQualityWeMos" AP.
    // Connect phone to that network — captive portal opens automatically.
    // Credentials are saved to flash; subsequent boots connect directly.
    WiFiManager wm;
    wm.setConfigPortalTimeout(180); // reboot after 3 min if unconfigured
    if (!wm.autoConnect("AirQualityWeMos")) {
        Serial.println(F("[WiFi] Portal timed out — rebooting."));
        delay(1000);
        ESP.restart();
    }
    Serial.printf("[WiFi] Connected — IP: %s\n",
                  WiFi.localIP().toString().c_str());

    // PMS5003 needs ~30 s to stabilise after power-on
    Serial.printf("[Setup] PMS5003 warm-up: %d s...\n", PMS_WARMUP_MS / 1000);
    delay(PMS_WARMUP_MS);

    if (!sensors.begin()) {
        Serial.println(F("[Setup] WARNING: no sensors initialised — check wiring."));
        ledSensorError();
    }

    Serial.println(F("[Setup] Ready."));
}

void loop() {
    Serial.println(F("\n[Loop] Reading sensors..."));
    ledReadingStart();

    PmsData pmsData = sensors.readPms();
    BmeData bmeData = sensors.readBme();

    ledOff();

    if (!pmsData.valid && !bmeData.valid) {
        Serial.println(F("[Loop] Both sensor reads failed."));
        ledSensorError();
    } else if (!pmsData.valid || !bmeData.valid) {
        // One sensor failed — blink error but still transmit the good data
        ledSensorError();
    }

    postReadings(pmsData, bmeData);

    Serial.printf("[Loop] Next reading in %lu s.\n", READING_INTERVAL_MS / 1000);
    delay(READING_INTERVAL_MS);
}
