# MKS XDRIVE Mini FOC Driver: Full Setup Guide & Firmware

<img width="995" height="611" alt="main" src="https://github.com/user-attachments/assets/7aef461c-1b51-4e2b-97c3-cc11ad7e6466" />


A comprehensive guide for the **MKS XDRIVE Mini**, a cost-effective, high-performance FOC driver for robotics. This repository contains configuration files, unbricking procedures, and multi-driver CAN bus setup instructions.

---

## 📺 Video Tutorial
Watch the step-by-step video guide here:
[**MKS XDRIVE Mini Full Instruction & Setup**](https://youtu.be/yRx7dsJmNvU?si=e_iXddEL_CQ9xSlw)

---

## 🚀 Quick Start (Windows)
Follow these steps to configure your driver using the ODrive tool:

1. **Install Python:** Download from python.org. 
   * **Important:** Select "Add python.exe to PATH" during installation.
2. **Install ODrive Tool:** Open PowerShell and install the ODrive package version 0.5.1.post0.
3. **Configuration:** Run **odrivetool** in your terminal. Use the commands provided in the **my_config.txt** file.
4. **Motor Tuning:** Ensure you set your pole pairs correctly. For example, for an LA8308 170kV motor, you would enter:
   **odrv0.axis0.motor.config.pole_pairs = 20**
   *Note: Calculate this by counting your motor magnets and dividing by 2.*

---

## 🛠 Hardware & Compatibility
* **Web GUI Interface:** For a visual setup, use this [Awesome WEB GUI](https://github.com/MoonLighTingPY/odrive3.6_web_gui).
* **Arduino/ESP32 Setup:** Ensure your board has CAN support.
    * For **ESP32**, place the **ODriveEsp32Twai.hpp** adapter in your arduino/libraries/OdriveArduino/src folder.
* **macOS Support:** odrivetool is functional on Mac, but the GUI is currently unsupported.

---

## ⚠️ Critical Notes & Troubleshooting

### 1. Avoid Bricking the Driver
* **Do NOT** use the "upgrade" command in odrivetool. This driver ships with modified **v0.5.1 firmware**; standard upgrades will brick the device.
* **Unbricking:** If bricked, you must use an **ST-Link programmer**. You can find dumped original firmware in this repository, you will need ST-linkV2 to unbrick.

### 2. CAN Bus Conflicts (Multi-Driver Setup)
By default, the MKS XDRIVE Mini acts like a dual-axis ODrive. This causes CAN ID conflicts when connecting multiple drivers.
* **The Fix:** Change the "ghost" Axis1 to a "listen-only" ID to clear the bus by setting:
  **odrv0.axis1.config.can_node_id = 63**

### 3. Firmware Versions
Current testing shows that updating to **v0.5.6** may result in motor inactivity (likely due to different pin assignments for the DRV8301). Stick to v0.5.1 for now. 

---

## 🔌 Wiring & Schematics
### ESP32-S3 Connection Diagram
<img width="2432" height="1155" alt="esp32_s3_two_drivers_schematic" src="https://github.com/user-attachments/assets/a599c1f9-8c19-401d-9544-9ee480b77bb5" />

---
*Disclaimer: This repository is a community-made guide. I have no affiliation with the manufacturer.*
