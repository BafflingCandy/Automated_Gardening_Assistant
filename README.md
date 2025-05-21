# 🌱 Automated Gardening Assistant

A smart IoT-based gardening system using an ESP32 microcontroller. This project automates plant watering based on real-time soil moisture, weather data, and sunlight cycles — all controlled via Home Assistant and MQTT.

---

## 🔧 Features

- **Smart Watering**  
  Waters only when soil is dry and rain is not expected.

- **Weather-Aware Logic**  
  Fetches data from Open-Meteo API to check precipitation probability.

- **MQTT Integration**  
  Dynamically adjust watering settings and location remotely.

- **Home Assistant Support**  
  Integrates with Home Assistant UI for real-time control and monitoring.

- **Deep Sleep Scheduling**  
  Puts ESP32 to sleep after sunset and wakes it at sunrise to save energy.

---

## ⚙️ Hardware Used

- ESP32 Microcontroller  
- Capacitive Soil Moisture Sensor  
- 5V DC Submersible Pump  
- Relay Module  

---

## 🖥️ Software Stack

- Arduino IDE  
- MQTT (Mosquitto Broker)  
- Home Assistant  
- Open-Meteo API  
- NTP (Time Sync)

---

## 📡 MQTT Topics

| Topic                  | Purpose                                  |
|------------------------|------------------------------------------|
| `home/sun_next_rising` | Updates sunrise timestamp                |
| `home/sun_next_setting`| Updates sunset timestamp                 |
| `home/moisture_threshold` | Sets soil moisture level threshold     |
| `home/water_time`      | Defines watering duration (seconds)      |
| `home/wait_time`       | Sets interval between checks (minutes)   |
| `home/location`        | Sets latitude and longitude              |
| `home/soil_moisture`   | Publishes current soil moisture level    |
| `home/pump_state`      | Publishes pump ON/OFF status             |
| `home/alert`           | Publishes rain or watering alerts        |

---

## 📄 Project Presentation

View the slide deck describing the project architecture and design:

📄 [View Project Presentation on Canva](https://www.canva.com/design/DAGjID-m564/V7o-oSWpB8TCcSt9B6y4TA/edit?utm_content=DAGjID-m564&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton)

---

## 📬 Contributions

Open to improvements, suggestions, and feature additions. Feel free to fork and build on this.

---

## 🛠 Author

[Pratyush Nagarad](https://www.linkedin.com/in/pratyush-nagarad)
