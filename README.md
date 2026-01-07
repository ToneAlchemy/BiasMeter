# Arduino Tube Amp Bias Meter (V11.4 Platinum)

This repository contains the complete source code and documentation for a professional-grade, dual-probe vacuum tube bias meter. Designed for guitar amplifier technicians and hobbyists, this tool allows for the precise measurement and setting of bias for power tubes (EL34, 6L6GC, 6V6, etc.) using a high-resolution color interface.

**Current Recommended Version:** V11.4 Platinum (Stable Standard Release)

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

> [!IMPORTANT]
> ### ⚡ Critical Safety: Capacitor Discharge
> Before touching any internal component or connecting probes to a chassis, you must verify the filter capacitors are drained.
>
> * **The Golden Rule:** Always measure with a multimeter to confirm 0VDC before touching anything.
> * **Discharge Tool:** Do not rely on the amp to drain itself. Use a dedicated **Capacitor Discharge Tool** to safely bleed off voltage.
>     * *DIY Tip:* You can build one using a high-wattage resistor (e.g., **20kΩ to 100kΩ, 5 Watts**) wired to a probe and an alligator clip for ground. This slows the discharge **and** prevents sparking, unlike a screwdriver short.
> * **One-Hand Rule:** When working on live amps, keep one hand in your pocket to prevent current from passing across your chest/heart.

> [!NOTE]
> ### 🛡️ Recommended Safety Gear
> If you are working inside a live amplifier chassis, we strongly recommend using the following safety equipment:
> 
> * **Isolation Transformer:** This isolates the amplifier from the mains earth ground, significantly reducing the risk of lethal shock if you accidentally touch a live component while grounded.
> * **Dim Bulb Limiter (Current Limiter):** Essential when powering up an amplifier for the first time after a repair. It prevents catastrophic damage (blown transformers) if there is a short circuit.
>      * **⚠️ CRITICAL WARNING:** Do **NOT** attempt to set the final bias while running through a Dim Bulb Limiter. The limiter drops the wall voltage, which lowers the plate voltage and gives false low bias readings. Always bypass the limiter for the final precision bias adjustment.
> * **Variac (Variable Transformer):** Highly recommended for two purposes:
>      1.  **Voltage Stabilization:** Wall voltage fluctuates. A Variac allows you to set the input voltage exactly to your country's standard (e.g., 240V in Australia, 120V in USA, 230V in UK). This ensures your bias readings are accurate and consistent, rather than drifting with high/low wall voltage.
>      2.  **Soft Start / Reforming:** Useful for slowly bringing up the voltage on vintage amplifiers to reform old capacitors safely.

## Project Evolution & Attribution

This project is the culmination of extensive development, evolving from earlier open-source concepts into a fully distinct firmware product.

* **98% Code Rewrite:** While inspired by earlier works, approximately **98% of the source code in this repository has been rewritten** from scratch. The underlying firmware—including display drivers, sensor management, memory architecture, safety logic, and the input state machine—is entirely new custom engineering.
* **Legacy:** This work is an adaptation of:
    1.  **"Dual Channel Arduino Bias Tester beta 14"** by Kiel Lydestad (3DBeerGoggles).
    2.  **"ArduinoBiasMeter"** by John Wagner.

## Firmware Versions & Downloads

This repository offers three distinct firmware versions. Choose the one that best fits your needs.

| Feature | **v11.4 (Recommended)** | **v11.4.1 (WTD Edition)** | **v11.3 (Legacy)** |
| :--- | :--- | :--- | :--- |
| **Stability Status** | **Gold Master (Standard)** | **Gold Master (Advanced)** | Legacy Stable |
| **Watchdog Timer** | ❌ Disabled | ✅ Enabled (Auto-Reset) | ❌ Disabled |
| **Data Protection** | ✅ Checksum + ID | ✅ Checksum + ID | ⚠️ Basic ID Only |
| **Safety Clamps** | ✅ Yes | ✅ Yes | ❌ No |
| **Limit Handling** | Freezes until safe (Limit - 50V) | Freezes until safe (Limit - 50V) | Freezes until safe (Limit - 50V) |
| **Tube Profiles** | **6 Default Profiles** | **6 Default Profiles** | 3 Default Profiles |
| **Best For...** | **Most Users.** Best balance of safety and usability. | **Safety Critical.** Auto-reboots if the CPU freezes. | Older Builds. |

### Which Version Should I Choose?

* **v11.4.1 (WTD Edition):** **Use this first.** This is the most robust version. It includes a "Watchdog Timer" that constantly monitors the processor. If electrical noise from the tube amp (sparks, EMI) causes the screen to freeze, the system will automatically reboot in 4 seconds. This is critical for safety.
* **v11.4 (Standard):** **Use this if v11.4.1 fails.** Some cheaper Arduino Nano clones have incompatible bootloaders that crash when the Watchdog Timer is used. If v11.4.1 gets stuck in a reboot loop on your device, switch to this version. It has all the same features but without the auto-reboot safety.
* **v11.3 (Legacy):** **Use only for backups.** This is an older stable release. It lacks the advanced EEPROM Checksum data protection and the expanded tube database of the v11.4 series.

### 🛠️ Technical Deep Dive: The Watchdog Timer (WTD)

**What is it?**
The Watchdog Timer (WTD) is a hardware safety feature inside the Arduino's processor. It acts like a "Dead Man's Switch." The main code must constantly signal the timer (pet the dog) to let it know the system is running correctly.

**Why is it important for Tube Amps?**
Tube amplifiers are electrically noisy environments. High voltage spikes, flyback EMF, or loose tube socket connections can create strong Electromagnetic Interference (EMI). This interference can sometimes freeze the I2C bus (the connection to the ADC) or lock up the display driver.
* **Without WTD (v11.4):** If the screen freezes due to noise, it stays frozen. You won't know if the reading is live or stuck.
* **With WTD (v11.4.1):** If the system freezes for more than 4 seconds, the Watchdog detects the lockup and automatically reboots the device, restoring live readings immediately.

### ⚠️ Troubleshooting: The "Boot Loop" Issue
Some older Arduino Nano boards (and many low-cost clones) come with an outdated "Bootloader" that has a bug. This bug prevents the chip from recovering correctly after a Watchdog Reset, causing the device to get stuck in an infinite reboot loop (blinking LED).

**If v11.4.1 causes your Nano to loop endlessly, you have two options:**
1.  **The Easy Fix:** Switch to **v11.4**. It is identical in features but has the WTD disabled to prevent this crash.
2.  **The Advanced Fix:** You can burn the modern **"Optiboot"** bootloader onto your Nano. This fixes the bug and allows you to use the WTD safety features.
    * *Note:* This requires an ISP programmer (like a **USBasp** or using a second **Arduino as ISP**). **Proceed with caution: Interrupting the burning process can brick your device.**
    * *Guide 1:* [Installing Optiboot to Fix WDT Issue](https://bigdanzblog.wordpress.com/2014/10/23/installing-the-optiboot-loader-on-an-arudino-nano-to-fix-the-watch-dog-timer-wdt-issue/)
    * *Guide 2:* [AVR Watchdog Timer and Arduino Nano](https://dvdoudenprojects.blogspot.com/2015/08/avr-watchdog-timer-and-arduino-nano.html)

* **[📂 Download Firmware Code Here](https://github.com/ToneAlchemy/BiasMeter/tree/main/BIASMETER-CODE)**

## 💻 Software & Development Environment

This project is written as a standard Arduino sketch (`.ino`) and was developed using the official **Arduino IDE**.

**Recommended Method:**
We strongly recommend using the **Arduino IDE** for the easiest setup. Its built-in *Library Manager* (Tools > Manage Libraries) makes it simple to install the required dependencies (Adafruit GFX, ADS1X15, etc.) without complex manual configuration.

**Alternative IDEs:**
Advanced users can compile this code in other environments, such as **Visual Studio Code (with PlatformIO)** or **Microchip Studio**. However, please note:
* You will likely need to manually manage library paths and dependencies.
* Depending on your environment, you may need to rename the source file from `.ino` to `.cpp` or adjust the structure.

**📥 Get the Software:**
You can download the latest version of the Arduino IDE here:
[**Download Official Arduino IDE**](https://www.arduino.cc/en/software)

## Features (V11.4 Highlights)

V11.4 introduces professional-grade data integrity and safety features not found in previous iterations or DIY alternatives.

### 🛠 Core Functionality
* **Dual Probe Measurement:** Simultaneous real-time monitoring of Tube A and Tube B.
* **True Plate Dissipation:** Calculates power (Watts) and dissipation percentage based on the specific tube type.
* **Screen Current Correction:** Implements industry-standard math to subtract estimated screen current, ensuring the displayed bias represents true *Plate* dissipation, not just Cathode current.

### 💾 Robust Data Integrity (New in V11.4)
* **Checksum Protection:** The EEPROM data now includes a computed checksum. If the memory is corrupted or a new chip is installed, the system detects the error and automatically restores safe default values, preventing dangerous behavior.
* **Safety Clamps:** The Calibration Menu now prevents users from accidentally setting dangerous values (e.g., setting a shunt resistor to 0.0Ω or a voltage scaler to 0), effectively "clamping" entries to safe, realistic ranges.

### 🛡️ Safety & Reliability
* **Active Over-Voltage Monitor:** Continuous safety checks will lock the interface and display a "DANGER" warning if probe voltage exceeds limits.
* **Safety Hysteresis:** The system requires a voltage drop of 50V (hysteresis) before resetting the safety lock, preventing rapid toggling near the limit.
* **Input Debouncing:** Advanced state-machine logic eliminates switch "recoil" and bounce, ensuring the cursor never jumps unintentionally.

### ⚙️ On-Board Calibration
* **Software Calibration:** Shunt resistance and Voltage Divider scaling can be adjusted via the screen menu and saved to EEPROM. You do not need to edit the source code to calibrate the unit.

## 🧠 How It Works (Theory of Operation)

For those interested in the engineering, here is how the Bias Meter safely measures high-voltage tube amplifiers.

### 1. Measuring Plate Voltage (The Voltage Divider)
The Arduino cannot measure 500V directly (it would fry instantly).
* **The Circuit:** Inside the probe (or unit), a **Voltage Divider** network scales the high voltage down to a safe range (0-5V).
* **The Math:** The device measures this low "safe" voltage and multiplies it by the **Voltage Scale Factor** (calibrated in software) to calculate the true High Voltage.
    * *Example:* 450V from the amp is stepped down to 4.5V for the ADC. The screen displays "450V".

### 2. Measuring Bias Current (The Shunt Resistor)
To measure current, we use **Ohm's Law** (`V = I × R`).
* **The Circuit:** The tube's cathode current flows through a precision **1Ω Shunt Resistor** to ground.
* **The Measurement:** The current flowing through the resistor creates a tiny voltage drop across it (e.g., 35mA of current creates 35mV).
* **The ADC:** The high-precision **ADS1115 ADC** measures this tiny voltage drop with extreme accuracy. The Arduino then converts that millivolt reading directly into milliamps (since 1mV = 1mA across a 1Ω resistor).

### 3. Calculating Dissipation (The Wattage)
Once the system knows the Voltage (V) and the Current (I), it calculates the Plate Dissipation in real-time.
* **Formula:** `Watts = Plate Voltage × Plate Current`
* **Screen Correction:** If enabled, the system first subtracts the estimated Screen Current from the total current to ensure the Wattage displayed is for the **Plate only**, providing the most accurate bias reading possible.

## Bill of Materials (BOM)

| Quantity | Component                     | Description                                      |
| :------- | :---------------------------- | :----------------------------------------------- |
| 1        | Arduino Nano   (Recommended: USB-C Version)                 | Microcontroller board (ATmega328P)               |
| 1        | Adafruit ST7735 1.8" TFT      | 160x128 Color TFT Display (SPI)                  |
| 1        | ADS1115 16-Bit ADC Module     | High-precision 4-channel ADC (I2C)               |
| 3        | Tactile Push Buttons          | Menu navigation (Left, Right, Center/Select)     |
| 1        | Project Enclosure             | 3D printed case (See below)                      |
| -        | Hook-up Wire                  | 22 AWG for internals                             |
| 2        | Bias Probes                   | Octal (8-pin) probes (e.g., TubeDepot Bias Scout or DIY) |


* **Download Link:** **[📂 Download BOM Files Here](HARDWARE%20-%20PCB%20KICAD/Bill%20of%20Materials%20(BOM)%20for%20the%20BIASMETER.txt)**

### 🛠️ Building the Probes (Recommended Kit)
We highly recommend the **Tube Depot Bias Scout Kit** for your hardware. It provides a professional, safe, and robust enclosure for the socket and resistor.

* **Product Link:** [Tube Depot Bias Scout Kit](https://www.tubedepot.com/products/tubedepot-bias-scout-kit)
* **Assembly Manual:** [Download PDF Instructions](https://s3.amazonaws.com/tubedepot-com-production/spree/attached_files/td_bias_scout_assy_manual_v3.2.pdf)

**Assembly Notes for this Project:**
1.  **The Resistor:** The kit includes a **1Ω 1W resistor**. This is perfect for this project and matches the default calibration (1.00Ω).
2.  **Wiring Colors:** If you follow the standard instructions, the output plugs will match this project's wiring guide:
    * **Red:** Plate Voltage (Connect to PCB `V_IN` via voltage divider).
    * **Black:** Ground (Connect to PCB `GND`).
    * **White:** Cathode Current (Connect to PCB `ADC_IN`).
3.  **Connection:** You can either cut the banana plugs off and wire them directly to your PCB/Enclosure, or install female banana jacks on your Bias Meter enclosure for a detachable probe.

## 3D Printed Enclosure

A custom enclosure has been designed to house the Arduino Nano, Display, and Buttons safely.

* **Design Platform:** TinkerCAD
* **Editable Source:** [Bias Meter Enclosure V11 Files](https://www.tinkercad.com/things/gKRXVebJSTF/edit?sharecode=gUTg7qwfboPwH2RXnympp6uQYQx4FWwdrr1dFZcTKMg) *(Note: You must be logged into Tinkercad to access this link)*
* **Ready-to-Print Files:** **[📂 Download STL Files Here](HARDWARE%20-%20PCB%20KICAD/BiasMeter_Enclosure_STL)**
  
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
![PCB Schematic](HARDWARE%20-%20PCB%20KICAD/BiasMeter_Schematic.png)

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

### 🔌 Probe Wiring (Octal Sockets)
If you are building your own probes, correct wiring is essential for the math to work.

* **Pin 3 (Plate):** Connects to the Voltage Divider.
* **Pin 8 (Cathode):** Connects to the Shunt Resistor (and Ground).
* **Pin 1 & 8:** Often tied together in 6L6/EL34 amps.

![Octal Tube Socket Pinout](https://upload.wikimedia.org/wikipedia/commons/a/a3/6V6_%2C_6L6_Tube_pin-out_.jpg)

*Figure: Standard Octal (8-Pin) Pinout (Bottom View).*

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

1.  **Library Dependencies:** Install the following via the Arduino IDE Library Manager:
    * `Adafruit GFX Library`
    * `Adafruit ST7735 and ST7789 Library`
    * `Adafruit ADS1X15`
2.  **Hardware Setup:**
    * **ADS1115:** Connect via I2C (SDA -> A4, SCL -> A5).
    * **TFT Display:** Connect via SPI (Pins 8, 9, 10, 11, 13 defined in code).
    * **Buttons:** Connect to Digital Pins 5 (Left), 6 (Right), and 7 (Center/Select).
3.  **Upload:**
    * **Recommended:** Flash `BiasMeter_V11.4.ino` to the Arduino Nano for the best balance of safety and stability.
    * **Advanced:** Flash `BiasMeter_V11.4.1_WTD.ino` if you require Watchdog Timer functionality.

## Usage Guide

### Main Menu
* **Navigation:** Use **Left/Right** to scroll through tube profiles.
* **Selection:** Press **Center** to select a tube and begin measuring.

### Tube Manager (Updated in v11.4)
* Select **"TUBE MANAGER"** from the main menu.
* **Add/Edit/Delete:** You can create new tube profiles or edit existing ones (Name, Max Dissipation, Screen % Factor) and delete tube profiles.
* **Navigation:** Press **Center** to advance between fields. Press **Left/Right** to adjust values.
* **Save:** Navigate to `[SAVE]` and press Center to write to permanent memory.
* **Data Safety:** V11.4 automatically calculates a checksum when you save, ensuring your custom tubes are protected against corruption.

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
6.  **Repeat** for Probe B.

#### B. Calibrating Shunt Resistors (Bias Scout Probes)
If you are using commercial probes like the **Tube Depot Bias Scout**, they typically have three banana plugs: Red, Black, and White.

1.  **Disconnect:** Remove the probes from the amplifier. The probe must be unplugged to measure resistance accurately.
2.  **Identify Plugs:**
    * **Black:** Common (Ground)
    * **White:** Cathode (Current Measurement)
    * **Red:** Plate (Voltage Measurement) - *Do not use for this step.*
3.  **Find DMM Lead Resistance (Critical for Accuracy):**
    * Turn your DMM to its lowest Resistance (Ω) setting.
    * Touch the Red and Black DMM probes firmly together.
    * Note the number (e.g., **0.2Ω**). This is your "Lead Resistance."
4.  **Measure Probe:** Connect one DMM lead to the **Black** plug and the other to the **White** plug of the bias probe. Write down the total resistance (e.g., **1.2Ω**).
5.  **Calculate Actual Value:** Subtract the Lead Resistance from the Total.
    * *Math:* `1.2Ω (Total) - 0.2Ω (Leads) = 1.00Ω (Actual)`
    * *Why?* For a 1.0Ω resistor, a 0.2Ω error is huge (20%)! Not subtracting it could lead you to bias your amp dangerously hot.
6.  **Adjust:** In the "CAL SETUP" menu, select **"Adj Shunt Res A"** and enter the **Actual Value** (e.g., 1.00).
7.  **Repeat** for Probe B.

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
* **Examples (Typical Values):**
    * **EL34:** 25 Watts
    * **6L6GC:** 30 Watts
    * **6V6:** 12-14 Watts (Check your specific tube's datasheet; older types are lower).
    * **KT88 / 6550:** 35-42 Watts
* **Usage:** If you set this to 25W and measure 17.5W of dissipation, the meter will display **70%**.

### 2. Screen % Factor (Accuracy Tuning)
This feature significantly improves accuracy compared to standard bias probes.
* **The Physics:** Standard bias probes measure **Cathode Current**, which is the sum of Plate Current + Screen Current. However, Bias should be calculated using only **Plate Current**.
* **The Solution:** The "Screen % Factor" subtracts a percentage of the total current to estimate the true Plate Current.
    * `True Plate Current = Measured Current - (Measured Current * ScreenFactor)`
    
* **Recommended Settings:**
    * **EL34:** ~13% (0.13) - Pentodes draw more screen current.
    * **KT88 / 6550:** ~6.0% (0.06) - High power beam tetrodes.
    * **6L6GC:** ~5.5% (0.055) - Beam Tetrodes draw less.
    * **EL84:** ~5.0% (0.05) - Miniature pentodes.
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
