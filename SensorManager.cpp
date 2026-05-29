#include "SensorManager.h"
#include "config.h"

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <PMS.h>

// File-scope sensor instances
static SoftwareSerial pmsSerial(PIN_PMS_RX, PIN_PMS_TX);
static PMS           pms(pmsSerial);
static PMS::DATA     pmsRaw;
static Adafruit_BME680 bme;

// ─────────────────────────────────────────────────────────────────────────────

bool SensorManager::begin() {
    // ── PMS5003 ──────────────────────────────────────────────────────────────
    pmsSerial.begin(PMS_BAUD);
    pms.passiveMode();      // request readings on demand
    _pmsOk = true;          // confirmed on first successful read
    Serial.println(F("[PMS5003] Serial started (passive mode)."));

    // ── BME680 ───────────────────────────────────────────────────────────────
    Wire.begin(PIN_SDA, PIN_SCL);
    if (!bme.begin(BME680_I2C_ADDR, &Wire)) {
        Serial.println(F("[BME680] Init failed — check wiring and I2C address."));
        _bmeOk = false;
    } else {
        // Settings from AirQualityPiSensors/Sensors.py equivalent
        bme.setTemperatureOversampling(BME680_OS_8X);
        bme.setHumidityOversampling(BME680_OS_2X);
        bme.setPressureOversampling(BME680_OS_4X);
        bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme.setGasHeater(0, 0);               // disable gas — not in schema
        Serial.println(F("[BME680] Initialized."));
        _bmeOk = true;
    }

    return _pmsOk || _bmeOk;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mirrors readPms() in AirQualityPiSensors/Sensors.py:
//   2 attempts; on first timeout, wake + re-enable passive mode then retry.
// ─────────────────────────────────────────────────────────────────────────────
PmsData SensorManager::readPms() {
    PmsData result = {0.0f, 0.0f, 0.0f, false};

    for (int attempt = 0; attempt < PMS_READ_RETRIES; attempt++) {
        pms.requestRead();
        if (pms.readUntil(pmsRaw, PMS_READ_TIMEOUT)) {
            result.pm1_0  = pmsRaw.PM_AE_UG_1_0;
            result.pm2_5  = pmsRaw.PM_AE_UG_2_5;
            result.pm10   = pmsRaw.PM_AE_UG_10_0;
            result.valid  = true;
            _pmsOk        = true;
            Serial.printf("[PMS5003] PM1.0=%.1f  PM2.5=%.1f  PM10=%.1f µg/m³\n",
                          result.pm1_0, result.pm2_5, result.pm10);
            return result;
        }

        Serial.printf("[PMS5003] Read timeout (attempt %d/%d)\n",
                      attempt + 1, PMS_READ_RETRIES);

        // Recovery: mirror Pi's pms5003.setup() reset — wake then re-enter passive
        if (attempt < PMS_READ_RETRIES - 1) {
            pms.wakeUp();
            delay(3000);
            pms.passiveMode();
        }
    }

    _pmsOk = false;
    Serial.println(F("[PMS5003] Read failed after retries."));
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mirrors readBme() in AirQualityPiSensors/Sensors.py:
//   BME_NUM_READINGS samples, BME_DELAY_MS between each, then average.
// ─────────────────────────────────────────────────────────────────────────────
BmeData SensorManager::readBme() {
    BmeData result = {0.0f, 0.0f, 0.0f, 0.0f, false};

    if (!_bmeOk) {
        return result;
    }

    float tempSum = 0.0f, humSum = 0.0f, presSum = 0.0f, altSum = 0.0f;

    for (int i = 0; i < BME_NUM_READINGS; i++) {
        if (!bme.performReading()) {
            Serial.printf("[BME680] Reading %d/%d failed.\n", i + 1, BME_NUM_READINGS);
            _bmeOk = false;
            return result;
        }
        tempSum += bme.temperature;
        humSum  += bme.humidity;
        presSum += bme.pressure / 100.0f;          // Pa → hPa
        altSum  += bme.readAltitude(SEA_LEVEL_HPA);

        if (i < BME_NUM_READINGS - 1) {
            delay(BME_DELAY_MS);
        }
    }

    result.temperature = tempSum / BME_NUM_READINGS;
    result.humidity    = humSum  / BME_NUM_READINGS;
    result.pressure    = presSum / BME_NUM_READINGS;
    result.altitude    = altSum  / BME_NUM_READINGS;
    result.valid       = true;

    Serial.printf("[BME680] Temp=%.1f°C  Humidity=%.1f%%  Pressure=%.2fhPa  Altitude=%.1fm\n",
                  result.temperature, result.humidity,
                  result.pressure,    result.altitude);
    return result;
}
