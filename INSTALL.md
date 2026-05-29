# Installation Guide

This guide walks through setting up AirQualityWeMos on a **WeMos D1 Mini (ESP8266)** and a **Windows PC**. All commands are shown in full — no prior Arduino or embedded experience is assumed.

---

## Contents

1. [What you need](#1-what-you-need)
2. [Wire the sensors](#2-wire-the-sensors)
3. [Install the CH341SER USB driver](#3-install-the-ch341ser-usb-driver)
4. [Set up Arduino IDE](#4-set-up-arduino-ide)
5. [Configure config.h](#5-configure-configh)
6. [Flash the firmware](#6-flash-the-firmware)
7. [Configure WiFi](#7-configure-wifi)
8. [Set up the Python receiver](#8-set-up-the-python-receiver)
9. [Open Windows Firewall ports](#9-open-windows-firewall-ports)
10. [Run the dashboard](#10-run-the-dashboard)
11. [Verify end-to-end](#11-verify-end-to-end)
12. [Troubleshooting](#12-troubleshooting)

---

## 1. What you need

**Hardware:**

| Item | Notes |
|---|---|
| WeMos D1 Mini | ESP8266EX, 4 MB flash, 3.3 V logic, 2.4 GHz WiFi |
| PMS5003 | Plantower particulate matter sensor |
| BME680 | Bosch environmental sensor (breakout board with I2C pins) |
| Jumper wires | Male-to-female recommended |
| Breadboard | Optional but useful for organising wires and combining the GND wires |
| Micro-USB cable | For flashing and power |
| Windows PC | Runs the Flask receiver and Streamlit dashboard |

**Software:**

| Software | Where to get it |
|---|---|
| Arduino IDE 2.x | https://www.arduino.cc/en/software |
| Python 3.9 or later | https://python.org/downloads |
| CH341SER USB driver | Included in `CH341SER/` folder in this repository |

---

## 2. Wire the sensors

> **Connect sensors before the first power-on after flashing.** `sensors.begin()` runs once at startup — if a sensor is not connected at boot it will not be read until the next reset.

### A note on TX and RX

TX means *transmit* and RX means *receive* — from the perspective of the device the label is printed on:

- The **sensor's TX** (it transmits data) must connect to the **WeMos's D5** (the WeMos receives data on D5)
- The **sensor's RX** (it receives commands) must connect to the **WeMos's D6** (the WeMos transmits on D6)

**Do not connect TX to TX or RX to RX.** Also, do not use the hardware TX and RX pins on the WeMos — those are occupied by the USB serial connection and will prevent the PMS5003 from reading. Use D5 and D6 only.

---

### PMS5003 (SoftwareSerial — D5 / D6)

| PMS5003 | WeMos D1 Mini | Notes |
|---|---|---|
| VCC (pin 1) | 5V | Sensor requires 5 V — do not use 3V3 |
| GND (pin 2) | GND | See GND note below |
| TX (pin 3) | **D5** (GPIO14) | Sensor transmits → WeMos receives |
| RX (pin 4) | **D6** (GPIO12) | WeMos transmits → sensor receives |

> **Power check:** The PMS5003 contains a small internal fan. It should be audible and spinning within a few seconds of power-on. If the fan is silent, the sensor has no 5 V — check your VCC wire.

---

### BME680 (I2C — D1 / D2)

| BME680 | WeMos D1 Mini | Notes |
|---|---|---|
| VIN | 3V3 | 3.3 V only — do not connect to 5V |
| GND | GND | See GND note below |
| SDA | **D2** (GPIO4) | I2C data |
| SCL | **D1** (GPIO5) | I2C clock |

---

### GND note

The WeMos D1 Mini has **only one GND pin** on its main header. Twist the PMS5003 GND wire and the BME680 GND wire together and insert them into the same breadboard row that connects to that GND pin.

---

## 3. Install the CH341SER USB driver

The WeMos D1 Mini uses a CH340 USB-to-serial chip that requires a driver on Windows.

1. Open **Device Manager** (right-click Start → Device Manager)
2. Plug the WeMos into the PC via USB
3. If it appears under *Other devices* as an unknown device, right-click it → **Update driver** → **Browse my computer** → point it to the `CH341SER/` folder in this repository
4. Once installed, the WeMos appears as a COM port (e.g. `COM3`) under *Ports (COM & LPT)*

---

## 4. Set up Arduino IDE

### 4a. Add the ESP8266 board package

1. Open Arduino IDE → **File → Preferences**
2. Paste this URL into *Additional boards manager URLs*:
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. **Tools → Board → Boards Manager** → search `esp8266` → install **esp8266 by ESP8266 Community** (latest version)

### 4b. Select the board

**Tools → Board → esp8266 → LOLIN(WEMOS) D1 R2 & mini**

Optionally set upload speed to **921600** (Tools → Upload Speed) to speed up flashing.

### 4c. Install libraries

**Tools → Manage Libraries** — search and install each of these:

| Library | Author | Minimum version |
|---|---|---|
| WiFiManager | tzapu | 2.0 |
| Adafruit BME680 Library | Adafruit | 2.0 |
| Adafruit BusIO | Adafruit | — (dependency of BME680) |
| ArduinoJson | Benoit Blanchon | 7.0 |
| PMS Library | Mariusz Kacki | 1.1 |

> When prompted to install dependencies for Adafruit BME680, click **Install All**.

> **ArduinoJson version:** Install version 7.x. The firmware uses `JsonDocument` which is not available in version 6.

---

## 5. Configure config.h

Open `AirQualityWeMos/config.h` in Arduino IDE or a text editor.

### Find your PC's IP address

Both the WeMos and the PC must be on the same network (the same WiFi router or hotspot). On the PC, run in PowerShell:

```powershell
ipconfig
```

Look for **IPv4 Address** under your WiFi adapter — for example `192.168.1.42`. This is the address the WeMos will POST to.

> The IP address changes each time the PC connects to a different network, or if the router assigns a new one via DHCP. Update `RECEIVER_HOST` and reflash whenever this happens.

### Update config.h

```cpp
#define RECEIVER_HOST   "192.168.1.42"   // ← replace with your PC's IP
```

Other values to review:

| Define | What to check |
|---|---|
| `BME680_I2C_ADDR` | Leave as `0x77`. If init fails after flashing, run the i2c_scanner sketch (see Troubleshooting) to find the correct address for your module. |
| `SEA_LEVEL_HPA` | Update to your local weather station's current mean sea-level pressure for accurate altitude readings. |
| `READING_INTERVAL_MS` | Default 900000 ms (15 minutes). Reduce to e.g. `60000` during testing and restore before leaving the device running. |

---

## 6. Flash the firmware

The ESP8266 must be put into bootloader (flash) mode manually before each upload. Sensor wires on D5, D6, and D3 interfere with the bootloader — disconnect them before flashing.

**Steps:**

1. **Disconnect** any wires connected to **D5**, **D6**, and **D3** on the WeMos
2. Connect a jumper wire from the **D3 pin** to the **GND rail** (or GND pin)
3. Press the **RST button** on the WeMos — this boots it into flash mode
4. In Arduino IDE, confirm the correct **COM port** is selected under **Tools → Port**
5. Click **Upload**
6. Wait — the IDE will compile, then upload. A successful upload ends with:
   ```
   Hash of data verified.
   ```
7. **Remove the D3→GND jumper wire**
8. Press **RST** again to boot the firmware normally
9. Reconnect all sensor wires (PMS5003 to D5/D6; BME680 SDA/SCL to D2/D1)

---

## 7. Configure WiFi

On first boot after a fresh flash, WiFiManager tries to connect to any previously saved network. If none is found, it creates a WiFi access point named **AirQualityWeMos** and waits up to 3 minutes for configuration.

**Steps:**

1. On any phone or laptop, connect to the **AirQualityWeMos** WiFi network
2. A captive portal should open automatically — if not, open a browser and navigate to `http://192.168.4.1`
3. Tap **Configure WiFi**, select your network SSID, enter the password, and tap **Save**
4. The WeMos reboots and connects — the Serial Monitor shows:
   ```
   [WiFi] Connected — IP: 192.168.x.x
   ```

**WiFi credentials are saved to ESP8266 flash** in a separate area that is not overwritten by firmware uploads. You will not need to reconfigure WiFi each time you reflash.

> **If your phone is the hotspot:** the phone cannot simultaneously act as a hotspot and join the WeMos AP to configure it. Use a laptop or second device to open the portal, or rely on previously saved credentials if the WeMos has connected to this network before.

> **2.4 GHz only:** The ESP8266 does not support 5 GHz WiFi. If using an Android hotspot, go to hotspot settings and set **AP Band → 2.4 GHz**. Some phones default to 5 GHz or "Auto", which the WeMos cannot use.

---

## 8. Set up the Python receiver

Run these commands in PowerShell from the project root:

```powershell
cd "AirQualityWeMosSensors\receiver"
pip install -r requirements.txt
python receiver.py
```

The receiver binds to `0.0.0.0:5000` and prints each accepted reading to the console.

**Verify it is running:**

```powershell
Invoke-RestMethod -Uri "http://127.0.0.1:5000/health"
```

Expected response: `{ "status": "ok" }`

**Keep this terminal open** — `receiver.py` must be running when the WeMos attempts to POST readings.

---

## 9. Open Windows Firewall ports

If the receiver is running but the WeMos cannot reach it, Windows Firewall is likely blocking the inbound connection. Run in **PowerShell as Administrator**:

```powershell
New-NetFirewallRule -DisplayName "AirQuality Receiver" -Direction Inbound -Protocol TCP -LocalPort 5000 -Action Allow -Profile Private
New-NetFirewallRule -DisplayName "AirQuality Dashboard" -Direction Inbound -Protocol TCP -LocalPort 8501 -Action Allow -Profile Private
```

---

## 10. Run the dashboard

In a separate PowerShell terminal:

```powershell
cd "AirQualityWeMosSensors\receiver"
streamlit run dashboard.py
```

Open a browser on any device on the same network:

```
http://<PC-IP-address>:8501
```

The dashboard auto-refreshes every 10 seconds.

---

## 11. Verify end-to-end

With `receiver.py` running and all sensors connected, open the Arduino IDE **Serial Monitor** at **115200 baud** and press **RST** on the WeMos.

Expected Serial Monitor output:

```
[Boot] AirQualityWeMos starting...
[WiFi] Connected — IP: 192.168.x.x
[Setup] PMS5003 warm-up: 30 s...
[PMS5003] Serial started (passive mode).
[BME680] Initialized.
[Setup] Ready.

[Loop] Reading sensors...
[PMS5003] PM1.0=x.x  PM2.5=x.x  PM10=x.x µg/m³
[BME680] Temp=xx.x°C  Humidity=xx.x%  Pressure=xxxx.xxhPa  Altitude=xx.xm
[HTTP] POST http://192.168.x.x:5000/readings
[HTTP] Success (201)
[Loop] Next reading in 900 s.
```

Expected receiver console output:

```
192.168.x.x - - [timestamp] "POST /readings HTTP/1.1" 201 -
```

---

## 12. Troubleshooting

---

### Upload: "Timed out waiting for packet header"

The ESP8266 did not enter flash mode.

- Make sure a wire connects **D3 to GND** *before* pressing RST
- **Disconnect wires from D5 and D6** — the PMS5003 serial lines interfere with the bootloader and will cause this error
- Press **RST**, then click **Upload** within a second or two
- Confirm the correct **COM port** is selected under Tools → Port

---

### BME680: "Init failed — check wiring and I2C address"

`sensors.begin()` could not find the BME680 on I2C. Two possible causes:

**Sensors were not connected at boot:**
Connect them and press **RST**. `sensors.begin()` runs on every boot — the BME680 will be found on the next reboot with sensors connected.

**Wrong I2C address:**
BME680 breakout boards vary — the address is either `0x76` or `0x77` depending on the module. To find the correct address:

1. Flash the i2c_scanner sketch at `i2c_scanner/i2c_scanner.ino` (same board and upload procedure)
2. Open Serial Monitor at 115200 baud and press RST
3. The output shows the address(es) found, e.g. `Device found at 0x77`
4. Update `BME680_I2C_ADDR` in `config.h` to match the detected address
5. Reflash the main sketch

---

### PMS5003: Read timeout

The WeMos is not receiving data from the PMS5003 on D5.

- Confirm the sensor is wired to **D5** and **D6**, not to the hardware **TX** and **RX** pins
- Confirm polarity: PMS5003 **TX → D5**, PMS5003 **RX → D6**
- Confirm **VCC is on the 5V pin** — the PMS5003 fan should be spinning; if silent, the sensor has no 5 V
- Check for loose connections — wiggle each wire gently

---

### PMS5003: All readings are 0.0 µg/m³

The sensor responded but returned zeros. This is normal during the first one or two readings while the fan and laser stabilise after power-on. If zeros persist across multiple reading cycles, confirm the sensor has airflow — it must not be sealed inside an enclosure.

---

### WiFiManager: "AutoConnect FAILED" — portal opens every boot

The WeMos cannot connect to the saved network.

- Confirm the SSID and password entered in the captive portal are correct
- Confirm the network is on the **2.4 GHz band** — the ESP8266 does not support 5 GHz
- On Android hotspots: open hotspot settings and set **AP Band → 2.4 GHz** (some devices default to 5 GHz or Auto)
- Once the WeMos successfully connects, credentials are saved to flash and used automatically on future boots

---

### WiFiManager captive portal — phone is the hotspot

The phone cannot simultaneously act as a hotspot and connect to the WeMos AP to configure it.

**Options:**
- Use a laptop or second device to connect to the **AirQualityWeMos** AP and open `http://192.168.4.1`
- If the WeMos has previously connected to this hotspot, it will find the saved credentials automatically and skip the portal

---

### receiver.py: `ModuleNotFoundError`

The Python dependencies are not installed in the active environment.

```powershell
cd "AirQualityWeMosSensors\receiver"
pip install -r requirements.txt
```

If you are using a virtual environment, activate it first before running `pip install` and `python receiver.py`.

---

### WeMos POST fails — connection refused or non-201 response

1. Confirm `receiver.py` is running: `Invoke-RestMethod -Uri "http://127.0.0.1:5000/health"` should return `{"status":"ok"}`
2. Confirm `RECEIVER_HOST` in `config.h` matches the PC's **current** IP address — this changes when the PC connects to a different network
3. Confirm both devices are on the **same network**
4. Add Windows Firewall inbound rules for ports **5000** and **8501** (see [step 9](#9-open-windows-firewall-ports))
5. Test reachability from PowerShell: `Invoke-RestMethod -Uri "http://<PC-IP>:5000/health"`
