#pragma once

// ── Sensor pins ───────────────────────────────────────────────────────────────
// PMS5003: SoftwareSerial
#define PIN_PMS_RX  D5  // GPIO14 — WeMos RX ← PMS5003 TX
#define PIN_PMS_TX  D6  // GPIO12 — WeMos TX → PMS5003 RX

// BME680: I2C (default ESP8266 pins)
#define PIN_SDA     D2  // GPIO4
#define PIN_SCL     D1  // GPIO5

// ── Onboard LED ───────────────────────────────────────────────────────────────
// D4 / GPIO2 — active-LOW (LOW = on, HIGH = off)
#define PIN_LED     D4

// ── BME680 settings ───────────────────────────────────────────────────────────
#define BME680_I2C_ADDR   0x77
#define SEA_LEVEL_HPA     1013.25f  // TODO: update to your local sea-level pressure

// ── PMS5003 settings ──────────────────────────────────────────────────────────
#define PMS_BAUD          9600
#define PMS_WARMUP_MS     30000     // 30 s warm-up on first boot
#define PMS_READ_TIMEOUT  5000      // ms to wait for a passive-mode read
#define PMS_READ_RETRIES  2

// ── BME680 averaging (mirrors AirQualityPiSensors Sensors.py) ─────────────────
#define BME_NUM_READINGS  5
#define BME_DELAY_MS      2000      // delay between each of the 5 readings

// ── Receiver endpoint ─────────────────────────────────────────────────────────
// TODO: set RECEIVER_HOST to your Windows PC's LAN IP address.
// Tip: run `ipconfig` on the PC and look for IPv4 Address under your WiFi adapter.
// For a stable setup, assign a static IP or a DHCP reservation to the PC.
#define RECEIVER_HOST   "192.168.1.42"
#define RECEIVER_PORT   5000
#define RECEIVER_PATH   "/readings"
#define HTTP_TIMEOUT_MS 10000

// ── Polling interval ──────────────────────────────────────────────────────────
#define READING_INTERVAL_MS  60000UL   // 1 minute (matches Pi timer)
