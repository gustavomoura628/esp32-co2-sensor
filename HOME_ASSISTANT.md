# Home Assistant Integration (Future)

## Overview

Once the sensors are working with custom firmware, the plan is to migrate to
ESPHome for Home Assistant integration. ESPHome handles sensor reading, MQTT
publishing, and auto-discovery — no custom code needed on the ESP32.

## Architecture

```
ESP32-C3 SuperMini + Sensors
            ↓
    MQTT Broker (Mosquitto)
            ↓
    Home Assistant
```

Home Assistant and Mosquitto will run on **manjaro-desktop0** (always-on desktop
in the bedroom, same network as the D-Link AP).

## ESPHome Configuration

The GPIO pins below are for the ESP32-C3 SuperMini specifically (not a regular
ESP32).

```yaml
i2c:
  sda: GPIO5
  scl: GPIO6

uart:
  rx_pin: GPIO20
  tx_pin: GPIO21
  baud_rate: 9600

sensor:
  - platform: mhz19
    co2:
      name: "CO2"
    automatic_baseline_calibration: false
    update_interval: 60s

  - platform: htu21d
    temperature:
      name: "Temperature"
    humidity:
      name: "Humidity"
    update_interval: 60s
```

## Setup Steps

1. Install Home Assistant on manjaro-desktop0 (Docker or native)
2. Install the Mosquitto MQTT broker add-on
3. Install the ESPHome add-on
4. Create a new device with the configuration above
5. Flash the ESP32 via ESPHome
6. Device auto-discovers in Home Assistant

## mDNS

For quick direct access without Home Assistant, the ESP32 can advertise itself
via mDNS (reachable at `hostname.local`). Useful during development. For the
full smart home setup with dashboards and history, use the ESPHome + MQTT route.
