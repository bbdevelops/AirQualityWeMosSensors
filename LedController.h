#pragma once
#include <Arduino.h>
#include "config.h"

// Onboard LED on D4 / GPIO2 is active-LOW.
// All functions below handle the inversion so call sites use intuitive names.

inline void ledBegin() {
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH); // off at startup
}

// Solid on — sensors are being read
inline void ledReadingStart() {
    digitalWrite(PIN_LED, LOW);
}

// Off — standby between readings
inline void ledOff() {
    digitalWrite(PIN_LED, HIGH);
}

// 3 rapid blinks — WiFi POST in progress
inline void ledWifiTx() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_LED, LOW);
        delay(100);
        digitalWrite(PIN_LED, HIGH);
        delay(100);
    }
}

// 5 slow blinks — sensor init or read error
inline void ledSensorError() {
    for (int i = 0; i < 5; i++) {
        digitalWrite(PIN_LED, LOW);
        delay(500);
        digitalWrite(PIN_LED, HIGH);
        delay(500);
    }
}
