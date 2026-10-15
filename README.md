# ⚡ Smart Electric Pole Monitoring System

An ESP32-based prototype that monitors an electric pole and its power line in real time. It detects **pole tilt / fall** and **line disconnection or over-current**, and shows the status using Red and Green LEDs.

> Final-year project, B.E. Electrical & Electronics Engineering, Bapuji Institute of Engineering & Technology, Davangere.

---

## 📌 Table of Contents
1. [Problem Statement](#-problem-statement)
2. [Objectives](#-objectives)
3. [Features](#-features)
4. [System Block Diagram](#-system-block-diagram)
5. [Components Used](#-components-used)
6. [Pin Connections](#-pin-connections)
7. [Circuit Details](#-circuit-details)
8. [Working Principle (A to Z)](#-working-principle-a-to-z)
9. [Physical Model](#-physical-model)
10. [Software Setup](#-software-setup)
11. [Arduino Code](#-arduino-code)
12. [Calibration](#-calibration)
13. [How to Demo / Test](#-how-to-demo--test)
14. [Output Table](#-output-table)
15. [Troubleshooting](#-troubleshooting)
16. [Applications](#-applications)
17. [Limitations](#-limitations)
18. [Future Scope](#-future-scope)
19. [Author](#-author)

---

## 🚨 Problem Statement
Electric poles can tilt or fall because of storms, accidents or soil erosion, and power lines can break or get disconnected. Today these faults are usually found only after a public complaint, a power cut or an accident. This causes long outages, damage and danger to life.

## 🎯 Objectives
- Detect tilting or falling of an electric pole automatically.
- Detect line disconnection (no current) and abnormal current (fault).
- Give a clear visual indication of the pole and line condition.
- Build a low-cost, easily extendable IoT-ready prototype.

## ✨ Features
- **Tilt detection** using the ADXL345 accelerometer
- **Current monitoring** using the ACS712 (5A) sensor
- **Line disconnection detection** (current ≈ 0 A)
- **Over-current / fault detection** (threshold: **0.3 A**)
- **Status LEDs:** Green = normal, Red = fault
- Runs on a **battery**, so it is portable
- Easy to add a cloud dashboard (ThingSpeak / Blynk) later

## 🧩 System Block Diagram

```
                +-------------------+
 ADXL345 -----> |                   | -----> Green LED (Normal)
 (Tilt, I2C)    |       ESP32       |
                |    (Processing)   | -----> Red LED (Fault)
 ACS712 ------> |                   |
 (Current)      +-------------------+
                          ^
        4V Battery -> 5V Boost Converter (Power)
```

## 🔧 Components Used

| # | Component | Quantity | Purpose |
|---|-----------|----------|---------|
| 1 | ESP32 DevKit | 1 | Main controller |
| 2 | ADXL345 accelerometer | 1 | Detects pole tilt (I2C) |
| 3 | ACS712 5A current sensor | 1 | Measures line current |
| 4 | Red LED | 1 | Fault indication |
| 5 | Green LED | 1 | Normal indication |
| 6 | 220 Ω resistor | 2 | Current limiting for LEDs |
| 7 | 10 kΩ resistor | 1 | Voltage divider (upper) |
| 8 | 20 kΩ resistor | 1 | Voltage divider (lower) |
| 9 | Push button | 1 | Creates a fault / current-increase condition |
| 10 | Small torch bulb (3 V–6 V) | 1 | Load for the line |
| 11 | 4 V battery | 1 | Power source |
| 12 | 5 V boost converter (fixed, USB type) | 1 | Steps battery up to 5 V |
| 13 | Breadboard and jumper wires | - | Connections |
| 14 | PVC pipes (poles) | - | Model poles |
| 15 | Aluminium wire | - | Simulated power line |

## 🔌 Pin Connections

### ADXL345 (I2C)
| ADXL345 | ESP32 |
|---------|-------|
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| CS | 3.3 V (selects I2C mode) |
| SDO | GND (address 0x53) |

### ACS712 (5A)
| ACS712 | Connection |
|--------|-----------|
| VCC | 5 V (from boost converter) |
| GND | Common GND |
| OUT | 10 kΩ → **GPIO 34**, with 20 kΩ from that point to GND (voltage divider) |
| IP+ / IP- | In series with the power line (battery → ACS712 → bulb) |

### LEDs
| LED | ESP32 |
|-----|-------|
| Green LED (+) | GPIO 25 via 220 Ω |
| Red LED (+) | GPIO 26 via 220 Ω |
| Both LED (−) | GND |

## 🔋 Circuit Details

**Why a voltage divider on the ACS712?**
The ACS712 runs on 5 V and its output sits around 2.5 V at zero current (up to about 4.5 V at +5 A). The ESP32 ADC pins accept a **maximum of 3.3 V**, so connecting the output directly can damage the board. The 10 kΩ / 20 kΩ divider scales the signal by 2/3:

```
V_gpio34 = V_out × 20k / (10k + 20k) = V_out × 0.667
```

So 2.5 V at the sensor becomes about 1.67 V at the ESP32 (safe).

**Power:** 4 V battery → 5 V boost converter → ESP32 (VIN / 5V pin) and ACS712 VCC.

**Load circuit (line):** Battery + → ACS712 IP+ → IP- → aluminium wire (line) → push button → torch bulb → battery −.

**Common ground:** Battery, ESP32, ACS712 and ADXL345 must all share the same GND.

## ⚙️ Working Principle (A to Z)

1. **Power on.** The battery supplies the boost converter, which gives a steady 5 V to the ESP32 and ACS712.
2. **Initialisation.** The ESP32 starts the I2C bus, initialises the ADXL345, sets up the LED pins and the ADC pin.
3. **Zero-current calibration.** At startup, with no load current, the ESP32 averages many ADC readings of the ACS712 to find its zero-current voltage.
4. **Tilt sensing.** The ADXL345 measures acceleration on X, Y and Z axes. When the pole is upright, gravity acts mainly on the vertical axis. The code calculates the tilt angle from these values:
   `tilt = acos(Z / √(X² + Y² + Z²))`
   If the tilt angle exceeds the set limit, the pole is marked as **tilted / fallen**.
5. **Current sensing.** The ACS712 produces a voltage proportional to line current (185 mV per ampere for the 5A version). The ESP32 reads it, undoes the divider scaling and converts it to amperes.
6. **Disconnection logic.** If the current is almost 0 A while the system is supposed to be energised, the line is considered **disconnected / broken**.
7. **Fault logic.** If the current rises above **0.3 A** (for example when the push button connects the extra load), it is flagged as a **current fault**.
8. **Decision.**
   - Tilt OK and current normal → **Green LED ON**
   - Tilt detected, or line disconnected, or over-current → **Red LED ON**
9. **Continuous loop.** Steps 4 to 8 repeat every few hundred milliseconds, and values are printed on the Serial Monitor.
10. **Optional IoT.** The same readings can be pushed to a cloud dashboard through ESP32 Wi-Fi.

## 🏗️ Physical Model
- PVC pipes act as electric poles.
- Aluminium wire strung between the poles acts as the power line.
- The ESP32, breadboard and sensors are mounted on the model base.
- The ADXL345 is fixed on the pole so that it tilts along with it.
- A push button and a small bulb act as the load, used to demonstrate a current increase or fault.

## 💻 Software Setup
1. Install the **Arduino IDE**.
2. Add ESP32 board support: *File → Preferences → Additional Boards Manager URLs* and add
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. *Tools → Board → Boards Manager* → install **esp32 by Espressif Systems**.
4. *Tools → Manage Libraries* → install:
   - **Adafruit ADXL345**
   - **Adafruit Unified Sensor**
5. Select board **ESP32 Dev Module** and the correct COM port.


## 🎚️ Calibration
- **Zero current:** At power-up keep the bulb circuit OFF (no current) so the code can record the true zero point of the ACS712.
- **Tilt axis:** If your ADXL345 is mounted with a different axis pointing upward, replace `e.acceleration.z` with the correct axis.
- **Tilt limit:** Change `TILT_LIMIT_DEG` (for example 20° to 45°) to suit your model.
- **Current limit:** Change `CURRENT_LIMIT_A` (set to 0.3 A for this build) depending on your bulb and battery.
- **No multimeter?** Compare readings in the Serial Monitor with the pole upright and with the bulb connected, and adjust thresholds so that normal and fault states are clearly separated.

## 🧪 How to Demo / Test
1. Power on with the pole upright and the line connected → **Green LED ON**.
2. **Tilt test:** Tilt the pole by hand beyond the limit → **Red LED ON**. Straighten it → Green again.
3. **Disconnection test:** Break the aluminium wire connection (or remove the bulb/wire) so no current flows → **Red LED ON**.
4. **Fault test:** Press the push button to add load, so the current goes above 0.3 A → **Red LED ON**.
5. Watch the Serial Monitor (115200 baud) for the live values.

## 📊 Output Table

| Pole Tilt | Line Current | Condition | Green LED | Red LED |
|-----------|--------------|-----------|-----------|---------|
| Normal | Normal | System healthy | ON | OFF |
| Tilted | Any | Pole tilted / fallen | OFF | ON |
| Normal | ≈ 0 A | Line disconnected | OFF | ON |
| Normal | > 0.3 A | Over-current fault | OFF | ON |

## 🛠️ Troubleshooting

| Problem | Possible cause and fix |
|---------|-----------------------|
| `ADXL345 not found` | Check SDA/SCL (GPIO 21/22), 3.3 V power, CS pin to 3.3 V |
| Current always 0 | Line not passing through IP+/IP-, or loose wire; check series connection |
| Current reading noisy | Add a 0.1 µF capacitor at the ADC pin and keep wires short |
| Wrong current values | Re-check divider (10k/20k) and `DIVIDER_FACTOR`; recalibrate zero |
| Red LED always ON | Tilt limit too low, wrong axis, or disconnected line |
| ESP32 resets | Battery weak; use a fully charged battery |
| Upload fails | Hold the BOOT button while uploading; check COM port |

## 🌍 Applications
- Smart city and smart grid monitoring
- Early warning for fallen poles after storms
- Detecting power line theft or breakage
- Rural and remote area distribution lines
- Railway electrification lines

## ⚠️ Limitations
- Prototype works at low voltage with a battery, not on real high-voltage lines.
- Alerts are local (LEDs); no remote notification yet.
- Single pole monitored.
- Accuracy depends on calibration since no precision meter is used.

## 🚀 Future Scope
- Send data to **ThingSpeak / Blynk** through ESP32 Wi-Fi
- **SMS / WhatsApp / app alerts** to the electricity board
- **GPS module** to report the exact location of the faulty pole
- Buzzer alarm
- Solar-powered operation
- Multiple poles with a central dashboard
- Machine-learning based fault prediction

## 👩‍💻 Author
**Vismaya H N**
B.E. Electrical & Electronics Engineering
Bapuji Institute of Engineering & Technology, Davangere

⭐ If you like this project, give the repo a star!
