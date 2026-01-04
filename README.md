# Arduino Tube Amp Bias Meter (V1 Platinum)

This repository contains the complete source code and documentation for a professional-grade, dual-probe vacuum tube bias meter. Designed for guitar amplifier technicians and hobbyists, this tool allows for the precise measurement and setting of bias for power tubes (EL34, 6L6GC, 6V6, etc.) using a high-resolution color interface.

**Current Version:** V1 Platinum (Stable Release)

> [!CAUTION]
> ## ⚠️ WARNING: HIGH VOLTAGE SAFETY
> **Tube amplifiers contain lethal high voltage (often 400V–500V+), even when powered off and unplugged.**
> This project involves building and using test equipment that interfaces directly with these dangerous circuits.
> * **Risk of Death:** Incorrect wiring of the bias probes or misuse of this device can expose you to lethal current.
> * **Filter Capacitors:** High voltage can remain stored in the amplifier's capacitors long after the power is cut. You must know how to safely drain filter capacitors before opening any chassis.
> * **Qualified Personnel Only:** If you are not trained in high-voltage safety or are uncomfortable working with live circuits, **do not build or use this device**. Take your amplifier to a qualified technician instead.
> **DISCLAIMER:** This project is provided "as is" with no warranty. The author assumes no liability for injury, death, or equipment damage resulting from the use or construction of this device. **Use entirely at your own risk.**


## Project Evolution & Attribution

This project is the culmination of extensive development, evolving from earlier open-source concepts into a fully distinct firmware product.

* **98% Code Rewrite:** While inspired by earlier works, approximately **98% of the source code in this repository has been rewritten** from scratch. The underlying firmware—including display drivers, sensor management, memory architecture, safety logic, and the input state machine—is entirely new custom engineering.
* **Legacy:** This work is an adaptation of:
    1.  **"Dual Channel Arduino Bias Tester beta 14"** by Kiel Lydestad (3DBeerGoggles).
    2.  **"ArduinoBiasMeter"** by John Wagner.

## Features (V1 Highlights)

Unlike previous iterations that required hard-coding values, V1 is a fully standalone tool with a dynamic operating system.

### 🛠 Core Functionality
* **Dual Probe Measurement:** Simultaneous real-time monitoring of Tube A and Tube B.
* **True Plate Dissipation:** Calculates power (Watts) and dissipation percentage based on the specific tube type.
* **Screen Current Correction:** Implements industry-standard math to subtract estimated screen current, ensuring the displayed bias represents true *Plate* dissipation, not just Cathode current.

### 💾 Dynamic Database System (New in V1)
* **EEPROM Tube Manager:** Users can **Add, Edit, Delete, and Save** tube profiles directly on the device using the onboard menu. No need to re-upload code to add a new tube type.
* **Memory Safety Clamps:** Built-in protection against memory corruption and buffer overflows.

### 🛡️ Safety & Reliability
* **Active Over-Voltage Monitor:** Continuous safety checks will lock the interface and display a "DANGER" warning if probe voltage exceeds limits.
* **Safety Hysteresis:** The system requires a voltage drop of 50V (hysteresis) before resetting the safety lock, preventing rapid toggling near the limit.
* **Input Debouncing:** Advanced state-machine logic eliminates switch "recoil" and bounce, ensuring the cursor never jumps unintentionally.

### ⚙️ On-Board Calibration
* **Software Calibration:** Shunt resistance and Voltage Divider scaling can be adjusted via the screen menu and saved to EEPROM. You do not need to edit the source code to calibrate the unit.

## Calibration Guide

The meter comes with **Default** calibration settings (Automatic), but for professional accuracy, we strongly recommend **Manual** calibration.

**How to Enter Calibration Mode:**
1.  **From Menu:** Scroll to the end of the menu and select **"CAL SETUP"**.
2.  **Startup Shortcut:** Press and hold the **Left Button** immediately when powering on (during the splash screen) to jump straight into Calibration Mode.

### Method 1: Default Calibration (Automatic)
When you first power on the device (or after a reset), it loads standard default values:
* **Voltage Scale:** Assumes standard resistors (e.g., 100K/10K divider). Deafults Set to 10.0
* **Shunt Resistor:** Assumes a perfect 1.00Ω resistor in the probe. Defaults Set to 1.00

**Use Case:** This is "Plug and Play." It works reasonably well if you used high-tolerance components (1% or better), but real-world resistors often vary (e.g., a 1Ω resistor might actually be 1.02Ω). **It is always good practice to check these settings against a multimeter anyway.**

### Method 2: Manual Calibration (Precision)
For the highest accuracy, use this method to match the meter readings to a trusted Digital Multimeter (DMM).

#### A. Calibrating Voltage Scale
1.  **Safety First:** Refer to your amplifier's schematic to find a safe test point for the B+ Voltage (Plate Voltage) that supplies Pin 3 of the power tubes.
2.  **Connect:** With the amp powered off and drained, connect the Bias Probe to the socket.
3.  **Measure:** Power on the amp. Use your DMM to measure the actual DC Voltage at the safe test point you identified.
4.  **Adjust:** In the Bias Meter "CAL SETUP" menu, select **"Adj Volt Scale A"**.
5.  **Match:** Use the Left/Right buttons to adjust the scale factor until the **"Live V"** on the screen matches the voltage shown on your DMM.
6.  Repeat for Probe B.

#### B. Calibrating Shunt Resistors (Bias Scout Probes)
If you are using commercial probes like the **Tube Depot Bias Scout**, they typically have three banana plugs: Red, Black, and White.

1.  **Disconnect:** Remove the probes from the amplifier. The probe must be unplugged to measure resistance accurately.
2.  **Identify Plugs:**
    * **Black:** Common (Ground)
    * **White:** Cathode (Current Measurement)
    * **Red:** Plate (Voltage Measurement) - *Do not use for this step.*
3.  **Measure:** Set your DMM to measure Resistance (Ohms) at its lowest setting. Connect one DMM lead to the **Black** plug and the other to the **White** plug.
4.  **Adjust:** In the "CAL SETUP" menu, select **"Adj Shunt Res A"**.
5.  **Match:** Adjust the value on screen to match the exact resistance you measured (e.g., if your DMM reads 1.02Ω, set the screen to 1.02).
6.  Repeat for Probe B.

#### C. Voltage Threshold Limiter
The **"Adj Voltage Limit"** setting is a safety tripwire.
* **How it works:** If the probe detects a voltage higher than this setting (default 600V), the device immediately triggers a Red "DANGER" screen and locks the interface.
* **Setting it:** Set this value slightly higher than your amplifier's maximum plate voltage (e.g., if your amp runs at 450V, set the limit to 500V or 550V). This protects the meter and warns you if the amp is behaving abnormally.
## Tube Manager Configuration Guide

The **Tube Manager** is powerful because it allows you to customize how the meter calculates Bias for specific tube types.

### 1. Max Dissipation (Watts)
This determines the "Red Line" for your tube. The meter uses this value to calculate the **% Dissipation** displayed on the screen.
* **How to set:** Look up the "Max Plate Dissipation" in the tube's datasheet.
* **Examples:**
    * EL34 = 25 Watts
    * 6L6GC = 30 Watts
    * 6V6 = 14 Watts
* **Usage:** If you set this to 25W and measure 17.5W of dissipation, the meter will display **70%**.

### 2. Screen % Factor (Accuracy Tuning)
This feature significantly improves accuracy compared to standard bias probes.
* **The Physics:** Standard bias probes measure **Cathode Current**, which is the sum of Plate Current + Screen Current. However, Bias should be calculated using only **Plate Current**.
* **The Solution:** The "Screen % Factor" subtracts a percentage of the total current to estimate the true Plate Current.
    * `True Plate Current = Measured Current - (Measured Current * ScreenFactor)`
* **Recommended Settings:**
    * **EL34:** ~13% (0.13) - Pentodes draw more screen current.
    * **6L6GC:** ~5.5% (0.055) - Beam Tetrodes draw less.
    * **6V6:** ~4.5% (0.045)
    * **Raw Mode:** Set to 0.00 to see the raw total current without subtraction.






## Bill of Materials (BOM)

| Quantity | Component                     | Description                                              |
| :------- | :---------------------------- | :------------------------------------------------------- |
| 1        | Arduino Nano                  | Microcontroller board (ATmega328P)                       |
| 1        | Adafruit ST7735 1.8" TFT      | 160x128 Color TFT Display (SPI)                          |
| 1        | ADS1115 16-Bit ADC Module     | High-precision 4-channel ADC (I2C)                       |
| 3        | Tactile Push Buttons          | Menu navigation (Left, Right, Center/Select)             |
| 1        | Project Enclosure             | 3D printed or off-the-shelf case                         |
| -        | Hook-up Wire                  | 22 AWG for internals                                     |
| 2        | Bias Probes                   | Octal (8-pin) probes (e.g., TubeDepot Bias Scout or DIY) |

## Installation

1.  **Library Dependencies:** Install the following via the Arduino IDE Library Manager:
    * `Adafruit GFX Library`
    * `Adafruit ST7735 and ST7789 Library`
    * `Adafruit ADS1X15`
2.  **Hardware Setup:**
    * **ADS1115:** Connect via I2C (SDA -> A4, SCL -> A5).
    * **TFT Display:** Connect via SPI (Pins 8, 9, 10, 11, 13 defined in code).
    * **Buttons:** Connect to Digital Pins 5 (Left), 6 (Right), and 7 (Center/Select).
3.  **Upload:** Flash `BiasMeter_V1.0_PLATINUM.ino` to the Arduino Nano.

## Usage Guide

### Main Menu
* **Navigation:** Use **Left/Right** to scroll through tube profiles.
* **Selection:** Press **Center** to select a tube and begin measuring.

### Tube Manager (New)
* Select **"TUBE MANAGER"** from the main menu.
* **Add/Edit/Delete:** You can create new tube profiles or edit existing ones (Name, Max Dissipation, Screen % Factor) and delete tube profiles.
* **Navigation:** Press **Center** to advance between fields. Press **Left/Right** to adjust values.
* **Save:** Navigate to `[SAVE]` and press Center to write to permanent memory.

### Calibration Mode
* Select **"CAL SETUP"** from the main menu.
* Use this mode to match the meter readings to a trusted multimeter.
* Adjust `V Scale` (Voltage) and `R Shunt` (Current) for both probes. Settings are saved to EEPROM automatically.

## Licensing

This work, **"Bias Meter"** by Charlie Isaac (ToneAlchemy.com), is licensed under a **Creative Commons Attribution 4.0 International License (CC BY 4.0)**.

### Attribution History
This project is an adaptation of:
* **"Dual Channel Arduino Bias Tester beta 14"** by Kiel Lydestad (3DBeerGoggles) (CC BY 4.0).
* **"ArduinoBiasMeter"** by John Wagner (CC BY 4.0).
