# Arduino Tube Amp Bias Meter

This repository contains the complete Arduino source code and documentation for a high-precision, dual-probe vacuum tube bias meter. This tool is designed for guitar amplifier technicians and hobbyists to accurately measure and set the bias of power tubes like the EL34 and 6L6GC.

The project features a user-friendly menu system, a color TFT display, and uses a high-resolution ADC for accurate readings. This code is the culmination of several iterations, building upon the work of earlier open-source bias meter projects.


## Project Overview

This bias meter allows a user to simultaneously measure the plate voltage and plate current of two separate power tubes. From these measurements, it calculates the plate dissipation in watts and displays it as a percentage of the tube's maximum rating, making it easy to dial in a precise bias setting (e.g., 70%).

The meter is built around an Arduino Nano, an Adafruit ST7735 TFT display, and a dual-channel ADS1115 analog-to-digital converter for precision.

## Features

* **Dual Probe Measurement:** Simultaneously measures two power tubes for easy pair matching.
* **Accurate Calculations:** Measures plate voltage and current, then calculates power (Watts) and dissipation percentage.
* **User-Friendly Interface:** A color TFT screen with a menu system allows for easy selection of different tube types.
* **Tube Library:** Pre-programmed with the specifications for common power tubes (EL34, 6L6GC, etc.).
* **Stable Readings:** Averages multiple ADC samples (`NUM_SAMPLES`) to provide a stable, noise-free reading.
* **Safety Features:** Includes a configurable `VOLTAGE_LIMIT` that will halt the program and display a warning if a dangerous voltage is detected.
* **Advanced Accuracy:** The code includes an approximation for screen grid current, resulting in a more accurate true plate dissipation reading.

## Bill of Materials (BOM)

This BOM covers the main components for the bias meter itself. It does not include the bias probes, which must be constructed or purchased separately.

| Quantity | Component                     | Description                                      |
| :------- | :---------------------------- | :----------------------------------------------- |
| 1        | Arduino Nano                  | Microcontroller board                            |
| 1        | Adafruit ST7735 1.8" TFT       | 160x128 Color TFT Display                        |
| 1        | ADS1115 16-Bit ADC Module     | High-precision analog-to-digital converter       |
| 3        | Tactile Push Buttons          | For menu navigation (Left, Right, Select)        |
| 1        | Project Enclosure             | A 3D printed or off-the-shelf case               |
| -        | Hook-up Wire                  | 22 AWG or similar for internal connections       |
| 2        | Bias Probes                   | Octal (8-pin) probes, e.g., TubeDepot Bias Scout |

## Installation & Setup

1.  **Libraries:** Ensure you have the following Arduino libraries installed through the Library Manager:
    * `Adafruit GFX Library`
    * `Adafruit ST7735 and ST7789 Library`
    * `Adafruit ADS1X15`
2.  **Hardware:** Assemble the components according to your chosen schematic. The display and ADC connect via I2C/SPI, and the buttons connect to digital pins.
3.  **Calibration:** For the highest accuracy, you must calibrate the code to your specific bias probes.
    * Use a quality multimeter to measure the true resistance of the resistors in your probes.
    * Update the values in the `CALIBRATION CONSTANTS` section of the `.ino` file before uploading.

## Usage

1.  Power on the meter. A splash screen will appear.
2.  Use the left and right buttons to scroll through the list of supported tube types.
3.  Press the center button to select the tube type you are measuring.
4.  The meter will switch to the bias reading screen, where it will display real-time measurements for Tube A and Tube B.
5.  Adjust the bias potentiometer in your amplifier until the desired percentage is reached.

## Licensing and Attribution

This work, "'Bias Meter'" by Tone Alchemy, is licensed under a **Creative Commons Attribution 4.0 International License (CC BY 4.0)**.

This project is an adaptation of:
* "'Dual Channel Arduino Bias Tester beta 14'" by Kiel Lydestad (3DBeerGoggles). (CC BY 4.0)
* "'ArduinoBiasMeter'" by John Wagner (CC BY 4.0)
```
