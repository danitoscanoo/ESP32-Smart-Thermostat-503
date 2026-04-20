# 🌡️ ESP32 Smart Thermostat (503 Wall Box Form Factor)

![ESP32](https://img.shields.io/badge/Hardware-ESP32-blue?style=for-the-badge&logo=espressif)
![C++](https://img.shields.io/badge/Language-C++-00599C?style=for-the-badge&logo=c%2B%2B)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge&logo=github)

## Project Overview
This repository contains the source code and hardware documentation for a custom-built, Wi-Fi connected smart thermostat. The entire system is engineered to be compact enough to fit inside a standard **Italian 503 electrical wall box**. 

It features an OLED physical display, a local Web UI, and autonomous temperature control based on NTP time scheduling, designed with extreme reliability in mind (incorporating Hardware Watchdog and Wi-Fi auto-recovery protocols).

## Key Features
* **Dual I2C Bus Architecture:** Independent I2C lines for the OLED display and the environmental sensor to prevent bus collisions and maximize stability.
* **Hybrid Control:** Monitor and adjust temperatures physically via the 1.1" OLED screen (with custom PROGMEM bitmaps) or remotely via a responsive local Web Server.
* **Smart Scheduling:** NTP-synchronized automatic modes (ECO during working hours, COMFORT during evenings) with manual override capabilities.
* **Fault-Tolerant:** Built-in hysteresis logic to protect boiler lifespan, backed by an ESP Hardware Watchdog Timer (WDT) and automated Wi-Fi reconnect routines.

## Specific Bill of Materials (BOM)
To replicate this project within the tight space of a 503 wall box, specific miniaturized components are required. The entire system has been meticulously designed around a **pure 3.3V ecosystem** for maximum energy efficiency, reduced heat dissipation, and simplified wiring.

| Component | Model / Specs | Description |
| :--- | :--- | :--- |
| **Microcontroller** | **ESP32-WROOM-32D** | Powerful ESP32 module with Wi-Fi capabilities. |
| **Power Supply** | **HLK-5M03** | 240V AC to 3.3V DC (5W) ultra-compact step-down module. |
| **Relay Module** | **1-Channel 3.3V (Low Level Trigger)** | Micro relay module operating entirely at 3.3V logic. |
| **Display** | **1.3" OLED Module (White)** | Crisp 1.3-inch OLED display communicating over IIC (4-Pin). |
| **Sensor** | **BMP280 (3.3V)** | High-precision Temperature & Barometric Pressure sensor (I2C). |

## Pinout & Wiring Configuration
**Important Power Note:** Since the system is fully 3.3V, the HLK-5M03 output MUST be connected to the `3V3` pin of the ESP32-WROOM-32D, *not* the `VIN` or `5V` pin.

| Component | Pin Type | ESP32 GPIO / Connection |
| :--- | :--- | :--- |
| **System Power** | 3.3V & GND | `3V3` & `GND` |
| **Relay Output** | Digital Out | `GPIO 26` *(Note: Low-Level Trigger)* |
| **BMP280 Sensor** | I2C Bus 1 | SDA `GPIO 21` \| SCL `GPIO 22` |
| **OLED Display** | I2C Bus 2 | SDA `GPIO 33` \| SCL `GPIO 32` |

## Pinout & Wiring
* **Relay Output:** `GPIO 26`
* **Sensor (I2C Bus 1):** SDA `GPIO 21` | SCL `GPIO 22`
* **OLED (I2C Bus 2):** SDA `GPIO 33` | SCL `GPIO 32`

## How to Install and Configure

Follow these steps to replicate the smart thermostat. 

### 1. Hardware Assembly
⚠️ **WARNING - HIGH VOLTAGE:** This project involves mains voltage (240V AC) connected to the HLK-5M03 module and the relay contacts. Ensure all mains power is **completely disconnected** from the breaker before wiring. If you are not qualified, consult a licensed electrician.

* Connect the HLK-5M03 step-down converter to the 240V mains, and route its 3.3V output to the `3V3` and `GND` pins of the ESP32.
* Connect the Relay, OLED, and BMP280 modules to their respective GPIOs as detailed in the Pinout section above.
* Ensure the Boiler control wires are connected to the **COM** and **NO** (Normally Open) terminals of the relay to ensure a fail-safe state (boiler OFF) in case of power loss.

### 2. Software Prerequisites
Ensure you have the Arduino IDE installed along with the **ESP32 Board Manager**. You will also need to install the following libraries via the Arduino Library Manager:
* `Adafruit GFX Library`
* `Adafruit SH110X` (For the 1.3" OLED)
* `Adafruit BMP280 Library`
* `Adafruit Unified Sensor`

### 3. Network Configuration
1. Open `Termostato_v1.1.ino` in your Arduino IDE.
2. Locate the User Configuration section at the top of the file.
3. Replace the placeholder Wi-Fi credentials with your actual network details:
   ```cpp
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";

### 4. Flashing and First Boot
1. Connect the ESP32 to your PC via USB.
2. Select your specific ESP32 board and COM port in the Arduino IDE.
3. Click **Upload**.
4. Once uploaded, the OLED display will show "Avvio sistema..." and attempt to connect to your Wi-Fi. 
5. Open the Serial Monitor (115200 baud) to find the local IP address assigned to the thermostat, or read it directly from your router's dashboard. Type this IP into any web browser on the same network to access the control interface.

### ⚡ System Architecture & Wiring Diagram

The diagram below illustrates the exact wiring of the 3.3V ecosystem. Notice how the I2C lines are split into two separate buses to prevent data collisions between the OLED and the BMP280.

```mermaid
graph TD
    %% Styling
    classDef power fill:#e74c3c,stroke:#c0392b,stroke-width:2px,color:#fff;
    classDef esp fill:#2980b9,stroke:#2c3e50,stroke-width:2px,color:#fff;
    classDef module fill:#27ae60,stroke:#2ecc71,stroke-width:2px,color:#fff;
    classDef relay fill:#f39c12,stroke:#d35400,stroke-width:2px,color:#fff;
    classDef boiler fill:#7f8c8d,stroke:#2c3e50,stroke-width:2px,color:#fff;

    %% Nodes
    MAINS["🔌 240V AC Mains"]:::power
    HLK["⚡ HLK-5M03 (3.3V Step-Down)"]:::power
    
    ESP["🧠 ESP32-WROOM-32D"]:::esp
    
    BMP["🌡️ BMP280 Sensor"]:::module
    OLED["🖥️ 1.3' OLED Display"]:::module
    
    RELAY["🧲 3.3V Relay Module (Low Level Trigger)"]:::relay
    BOILER["🔥 Boiler / Caldaia"]:::boiler

    %% Power Connections (240V)
    MAINS -->|L & N| HLK

    %% Power Connections (3.3V)
    HLK -->|+3.3V DC| ESP
    HLK -.->|GND| ESP
    
    HLK -->|+3.3V DC| RELAY
    HLK -.->|GND| RELAY
    
    HLK -->|+3.3V DC| BMP
    HLK -.->|GND| BMP
    
    HLK -->|+3.3V DC| OLED
    HLK -.->|GND| OLED

    %% Data Connections
    ESP -->|GPIO 26| RELAY
    
    ESP -->|GPIO 21 (SDA)| BMP
    ESP -->|GPIO 22 (SCL)| BMP
    
    ESP -->|GPIO 33 (SDA)| OLED
    ESP -->|GPIO 32 (SCL)| OLED

    %% Physical Output
    RELAY ===|COM & NO (Dry Contacts)| BOILER
```

## 🚧 Known Issues & Future Improvements
* **Thermal Isolation:** The BMP280 is highly sensitive; isolating it from the heat generated by the 240V power supply inside the wall box is critical to avoid false temperature readings. Future revisions may use an external probe.
* **Custom 3D Printed Enclosure:** To improve the physical integration within the 503 box, a future goal is to design and 3D print a custom case that snaps perfectly into a standard **Bticino Living** wall plate support, creating a seamless, professional finish.
