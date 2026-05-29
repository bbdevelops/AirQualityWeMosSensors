#pragma once
#include <Arduino.h>

struct PmsData {
    float pm1_0;
    float pm2_5;
    float pm10;
    bool  valid;
};

struct BmeData {
    float temperature;
    float humidity;
    float pressure;   // hPa
    float altitude;   // metres
    bool  valid;
};

class SensorManager {
public:
    // Initialise both sensors. Returns true if at least one is ready.
    bool begin();

    // Read PMS5003 — up to PMS_READ_RETRIES attempts.
    // Mirrors readPms() in AirQualityPiSensors/Sensors.py.
    PmsData readPms();

    // Read BME680 — averages BME_NUM_READINGS samples.
    // Mirrors readBme() in AirQualityPiSensors/Sensors.py.
    BmeData readBme();

    bool pmsOk() const { return _pmsOk; }
    bool bmeOk() const { return _bmeOk; }

private:
    bool _pmsOk = false;
    bool _bmeOk = false;
};
