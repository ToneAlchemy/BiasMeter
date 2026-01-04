# Arduino Tube Amp Bias Meter (V11.3 Platinum)

This repository contains the complete source code and documentation for a professional-grade, dual-probe vacuum tube bias meter. Designed for guitar amplifier technicians and hobbyists, this tool allows for the precise measurement and setting of bias for power tubes (EL34, 6L6GC, 6V6, etc.) using a high-resolution color interface.

**Current Version:** V11.3 Platinum (Stable Release)

> [!CAUTION]
> ## ⚠️ WARNING: HIGH VOLTAGE SAFETY
>
> **Tube amplifiers contain lethal high voltage (often 400V–500V+), even when powered off and unplugged.**
>
> This project involves building and using test equipment that interfaces directly with these dangerous circuits.
> * **Risk of Death:** Incorrect wiring of the bias probes or misuse of this device can expose you to lethal current.
> * **Filter Capacitors:** High voltage can remain stored in the amplifier's capacitors long after the power is cut. You must know how to safely drain filter capacitors before opening any chassis.
> * **Qualified Personnel Only:** If you are not trained in high-voltage safety or are uncomfortable working with live circuits, **do not build or use this device**. Take your amplifier to a qualified technician instead.
>
> **DISCLAIMER & NO SUPPORT:** This project is provided "as is" with **no warranty and no technical support**. The author cannot provide assistance with assembly, troubleshooting, or code modifications. The author assumes no liability for injury, death, or equipment damage resulting from the use or construction of this device. **Use entirely at your own risk.**

## Project Evolution & Attribution

This project is the culmination of extensive development, evolving from earlier open-source concepts into a fully distinct firmware product.

* **98% Code Rewrite:** While inspired by earlier works, approximately **98% of the source code in this repository has been rewritten** from scratch. The underlying firmware—including display drivers, sensor management, memory architecture, safety logic, and the input state machine—is entirely new custom engineering.
* **Legacy:** This work is an adaptation of:
    1.  **"Dual Channel Arduino Bias Tester beta 14"** by Kiel Lydestad (3DBeerGoggles).
    2.  **"ArduinoBiasMeter"** by John Wagner.

## Features (V11.3 Highlights)

Unlike previous iterations that required hard-coding values, V11.3 is a fully standalone tool with a dynamic operating system.

### 🛠 Core Functionality
* **Dual Probe Measurement:** Simultaneous real-time monitoring of Tube A and Tube B.
* **True Plate Dissipation:** Calculates power (Watts) and dissipation percentage based on the specific tube type.
* **Screen Current Correction:** Implements industry-standard math to subtract estimated screen current, ensuring the displayed bias represents true *Plate* dissipation, not just Cathode current.

### 💾 Dynamic Database System (New in V11.3)
* **EEPROM Tube Manager:** Users can **Add, Edit, Delete, and Save** tube profiles directly on the device using the onboard menu. No need to re-upload code to add a new tube type.
* **Memory Safety Clamps:** Built-in protection against memory corruption and buffer overflows.

### 🛡️ Safety & Reliability
* **Active Over-Voltage Monitor:** Continuous safety checks will lock the interface and display a "DANGER" warning if probe voltage exceeds limits.
* **Safety Hysteresis:** The system requires a voltage drop of 50V (hysteresis) before resetting the safety lock, preventing rapid toggling near the limit.
* **Input Debouncing:** Advanced state-machine logic eliminates switch "recoil" and bounce, ensuring the cursor never jumps unintentionally.

### ⚙️ On-Board Calibration
* **Software Calibration:** Shunt resistance and Voltage Divider scaling can be adjusted via the screen menu and saved to EEPROM. You do not need to edit the source code to calibrate the unit.

## Bill of Materials (BOM)

| Quantity | Component                     | Description                                      |
| :------- | :---------------------------- | :----------------------------------------------- |
| 1        | Arduino Nano   (Recommended: USB-C Version)               | Microcontroller board (ATmega328P)               |
| 1        | Adafruit ST7735 1.8" TFT      | 160x128 Color TFT Display (SPI)                  |
| 1        | ADS1115 16-Bit ADC Module     | High-precision 4-channel ADC (I2C)               |
| 3        | Tactile Push Buttons          | Menu navigation (Left, Right, Center/Select)     |
| 1        | Project Enclosure             | 3D printed case (See below)                      |
| -        | Hook-up Wire                  | 22 AWG for internals                             |
| 2        | Bias Probes                   | Octal (8-pin) probes (e.g., TubeDepot Bias Scout or DIY) |


* **Download Link:** **[📂 Download BOM Files Here](HARDWARE%20-%20PCB%20KICAD/Bill%20of%20Materials%20(BOM)%20for%20the%20BIASMETER.txt)

## 3D Printed Enclosure

A custom enclosure has been designed to house the Arduino Nano, Display, and Buttons safely.

* **Design Platform:** TinkerCAD
* **Download Link:** [Bias Meter Enclosure V11 Files](https://www.tinkercad.com/things/gKRXVebJSTF/edit?sharecode=gUTg7qwfboPwH2RXnympp6uQYQx4FWwdrr1dFZcTKMg)
* **Download Link:** **[📂 Download STL Files Here](HARDWARE%20-%20PCB%20KICAD/BiasMeter_Enclosure_STL)**
  
**Printing Notes:**
You can export the `.STL` files directly from the link above. Standard PLA or PETG is suitable for this project. The case is designed to keep the low-voltage electronics isolated and secure.

## PCB Design & Fabrication

For a cleaner build and enhanced reliability, a custom PCB has been designed to replace hand-wiring. This board hosts the Arduino Nano, ADS1115, and input connectors.

### Key Hardware Features
* **Hardware Input Protection:** The board includes **1N4733A Zener Diodes (5.1V)** on the probe inputs. These clamp any incoming voltage spikes to ~5.1V, protecting the sensitive ADS1115 ADC from damage in case of a tube failure or surge.
* **Simplified Assembly:** Eliminates the "rat's nest" of wires common in DIY builds.
* **Fabrication Ready:** You can use the provided Gerber/KiCad files to order these boards from any fabrication house (e.g., PCBWay, JLCPCB, OSH Park).

### PCB Board Views
![PCB Board Layout](https://github.com/ToneAlchemy/BiasMeter/raw/23a8e443d3858fa2ae11c6348a671421652070ce/HARDWARE%20-%20PCB%20KICAD/BiasMeter_InputProtection.png)

### PCB Board Components Views
![PCB Board Layout - Componets ](HARDWARE%20-%20PCB%20KICAD/BiasMeter_InputProtection_v3_3dView.png)

### Schematic layout
![PCB Schematic](https://github.com/ToneAlchemy/BiasMeter/raw/23a8e443d3858fa2ae11c6348a671421652070ce/HARDWARE%20-%20PCB%20KICAD/BiasMeter_Schematic.png)

* **Download Link:** **[📂 Download PCB & KiCad Files Here](HARDWARE%20-%20PCB%20KICAD/BiasMeter%20-%20KICAD)**


## Wiring / Pinout Guide

### Arduino Nano Connections
| Component | Pin Name | Nano Pin |
| :--- | :--- | :--- |
| **Buttons** | Left Button | D5 |
| | Right Button | D6 |
| | Select/Center | D7 |
| **Display (ST7735)** | CS | D10 |
| | DC (A0) | D9 |
| | RST | D8 |
| | SDA (MOSI) | D11 |
| | SCK (SCLK) | D13 |
| **ADC (ADS1115)** | SDA | A4 |
| | SCL | A5 |
| | VCC | 5V |
| | GND | GND |

### Arduino Nano Pinout 

![Arduino Nano Pinout](https://github.com/ToneAlchemists/BiasMeter/blob/25a091382b33c8da140eac9d3a7a2a432e6024f1/IMAGES/nano_pinout.jpg)

## Powering the Device (Crucial Safety Info)

Correctly powering the Arduino Nano is critical to prevent damage to your computer, the bias meter, or the amplifier.

### 1. Recommended: Separate USB Power Adapter
The safest way to power this device during use is with a dedicated USB wall charger (like a standard phone charger).
* **Specs:** 5V Output, at least 500mA (0.5A).
* **Connector:** Mini-USB or USB-C (for Nano). Recommend get USB-C Arduino Nano
* **⚠️ IMPORTANT - 2-Prong Only:** You must use a charger that has only **2 prongs** (ungrounded).
    * **Why?** Tube amplifiers are grounded to Earth. If you use a grounded (3-prong) power adapter for the Arduino, you may create a **Ground Loop**. This can introduce noise, cause erratic readings, or in worst-case scenarios, short high voltage to ground through your low-voltage electronics. A 2-prong adapter is "floating," which isolates the meter from Earth ground.

### 2. Battery Power (9V)
You can power the Arduino Nano via the `VIN` and `GND` pins using a 9V battery.
* **Pros:** Complete electrical isolation (100% safe from ground loops), portable, zero mains noise.
* **Cons:** The Arduino, Display, and ADC consume current constantly. A standard 9V battery may drain relatively quickly. The Nano's onboard regulator may get warm dropping 9V to 5V.

### 3. ⚠️ WARNING: Computer USB Port
**DO NOT** power the bias meter from your computer's USB port while it is connected to an amplifier.
* **The Danger:** Desktop computers (and many laptops) are grounded. Connecting the USB cable ties the Arduino's Ground to the Computer's Earth Ground. Since the Probe's Ground is connected to the Amp's Ground, you create a direct Ground Loop.
* **Risk:** If a probe accident occurs or there is a voltage potential difference, high current could flow through the USB cable, destroying your computer's motherboard, the Arduino, or the amplifier.

#### When is Computer USB safe?
* **Uploads:** It is safe to connect to the computer to upload code **ONLY IF** the bias probes are **disconnected** from the amplifier.
* **Debugging:** You can use the Arduino IDE Serial Monitor to view debug logs, but **ensure the probes are NOT connected to a live amplifier**. If you must debug a live circuit, you must use a **USB Isolator** to protect your equipment.

## Installation

* **Download Link:** **[📂 Download Firmware Code Here](https://github.com/ToneAlchemy/BiasMeter/tree/main/BIASMETER-CODE)**
  
1.  **Library Dependencies:** Install the following via the Arduino IDE Library Manager:
    * `Adafruit GFX Library`
    * `Adafruit ST7735 and ST7789 Library`
    * `Adafruit ADS1X15`
2.  **Hardware Setup:**
    * **ADS1115:** Connect via I2C (SDA -> A4, SCL -> A5).
    * **TFT Display:** Connect via SPI (Pins 8, 9, 10, 11, 13 defined in code).
    * **Buttons:** Connect to Digital Pins 5 (Left), 6 (Right), and 7 (Center/Select).
3.  **Upload:** Flash `BiasMeter_V11.3_PLATINUM.ino` to the Arduino Nano.

## Usage Guide

### Main Menu
* **Navigation:** Use **Left/Right** to scroll through tube profiles.
* **Selection:** Press **Center** to select a tube and begin measuring.

### Tube Manager (New)
* Select **"TUBE MANAGER"** from the main menu.
* **Add/Edit/Delete:** You can create new tube profiles or edit existing ones (Name, Max Dissipation, Screen % Factor) and delete tube profiles.
* **Navigation:** Press **Center** to advance between fields. Press **Left/Right** to adjust values.
* **Save:** Navigate to `[SAVE]` and press Center to write to permanent memory.

## Calibration Guide

The meter comes with **Default** calibration settings (Automatic), but for professional accuracy, we strongly recommend **Manual** calibration.

**How to Enter Calibration Mode:**
1.  **From Menu:** Scroll to the end of the menu and select **"CAL SETUP"**.
2.  **Startup Shortcut:** Press and hold the **Left Button** immediately when powering on (during the splash screen) to jump straight into Calibration Mode.

### Method 1: Default Calibration (Automatic)
When you first power on the device (or after a reset), it loads standard default values:
* **Voltage Scale:** Defaults to **10.00**. (Assumes standard 1MΩ/100Ω probe divider).
* **Shunt Resistor:** Defaults to **1.00**. (Assumes a perfect 1.00Ω resistor).

**Use Case:** This is "Plug and Play." It works reasonably well for standard probes, but real-world resistors often vary slightly. **It is always good practice to check these settings against a multimeter anyway.**

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

### 4. Saving & Exiting
When you are finished calibrating, simply scroll to **"[EXIT]"** (or **"[BACK]"**) in the menu and press the **Center Button**. This ensures all your new settings are permanently stored in the EEPROM memory.

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

## Licensing

This project, **"Bias Meter"** by Charlie Isaac (ToneAlchemy.com), is released under the **GNU General Public License v3.0 (GPLv3)**.

### What this means:
* **Open Source:** You are free to use, modify, and distribute this software.
* **ShareAlike:** If you modify this project and distribute it (or a product based on it), you **MUST** release your modifications under the same GPLv3 license. You cannot close-source this project.
* **No Warranty:** This software is provided "as is" without warranty of any kind.

### Attribution & Legacy
This project is an evolution of earlier open-source works released under the **CC BY 4.0** license. In accordance with that license, we gratefully acknowledge the original authors:
* Based on **"Dual Channel Arduino Bias Tester beta 14"** by Kiel Lydestad (3DBeerGoggles).
* Based on **"ArduinoBiasMeter"** by John Wagner.
