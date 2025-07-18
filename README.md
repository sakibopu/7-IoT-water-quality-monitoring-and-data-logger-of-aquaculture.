
# 🐟 IoT-Based Smart Water Quality Monitoring System for aquaculture

An end-to-end embedded system designed to **remotely monitor and log six critical water quality parameters** in real time for aquaculture environments such as fish farms and research ponds.

---

## 🎯 Project Objectives

To continuously and accurately track the following environmental parameters:

* **pH** – Acidity/alkalinity level
* **Dissolved Oxygen (DO)** – in µg/L
* **Electrical Conductivity (EC)** – in µS/cm
* **Turbidity** – in NTU (water clarity)
* **Temperature** – in °C
* **Total Dissolved Solids (TDS)** – in ppm

These metrics are crucial for maintaining healthy aquatic ecosystems and improving fish farm management.

---

## ⚙️ System Overview

### ✅ Hardware Design

* Custom-designed circuit board with efficient **GPIO allocation**, **power routing**, and **level shifters** to ensure seamless integration of **analog sensors** and **GSM communication**.
* Utilized a **4G LTE GSM module** to transmit data via HTTP due to lack of Wi-Fi in rural farm environments.
* Sensors were individually **calibrated using buffer and reference solutions** to ensure data accuracy.
* Developed a robust, **waterproof enclosure** for field deployment, safeguarding electronics against weather and environmental exposure.
* Built a **sensor transducer holder** to ensure correct positioning and stability in the pond water.

### 🖥️ Firmware & Connectivity

* Unified all sensor readings into a single embedded C/C++ codebase (Arduino-compatible).
* Implemented **HTTP POST requests** to send data to **Google Sheets via Google Apps Script**, allowing real-time remote access and logging.

---

## 📡 Deployment Status

The device is **fully deployed** and operational in a live fish farming environment.
It successfully transmitted its first dataset on day one and continues to push real-time sensor data to the cloud. Data collected over the next several months will aid in **research**, **environmental analysis**, and **smart farm decision-making**.

---

## 🌱 Impact & Takeaways

This project demonstrates the power of **IoT and embedded systems** in promoting **sustainable agriculture**.
By building this solution from scratch—including hardware, firmware, cloud integration, and enclosure—it serves as a practical, real-world example of how engineering can directly contribute to environmental monitoring and resource efficiency.

---

## 🔧 Tech Stack

* **Microcontroller**: Arduino-based board
* **Communication**: 4G LTE GSM Module (SIM7600/SIM800)
* **Cloud Logging**: Google Sheets via Apps Script
* **Programming Language**: C/C++ (Arduino)
* **Sensors**: Analog (pH, DO, EC, Turbidity, Temperature, TDS)

---

## 🚀 Future Improvements

* Add local SD card backup
* Implement alert notifications (SMS/email)
* Upgrade to solar-powered operation
* Web dashboard with visual analytics

