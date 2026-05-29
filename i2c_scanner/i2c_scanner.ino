// I2C Scanner — flash this to find your BME680 address
// Open Serial Monitor at 115200 baud

#include <Wire.h>

void setup() {
    Serial.begin(115200);
    delay(500);
    Wire.begin(4, 5); // SDA=D2(GPIO4), SCL=D1(GPIO5)
    Serial.println("\nI2C Scanner — scanning...");

    byte found = 0;
    for (byte addr = 8; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Device found at 0x%02X\n", addr);
            found++;
        }
    }

    if (found == 0) {
        Serial.println("  No I2C devices found — check SDA/SCL wiring and power.");
    } else {
        Serial.printf("  %d device(s) found.\n", found);
    }
    Serial.println("Done.");
}

void loop() {}
