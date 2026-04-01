# esphome-lampsmart-hf-light

ESPHome external component for HF-protocol BLE lamps that are controlled entirely through non-connectable BLE advertisements.

This component packages the validated HF-protocol BLE light implementation as an ESPHome `external_components` light platform. It supports:

- Pair
- Turn on / turn off
- Brightness
- Cold/warm white color temperature

## Requirements

- ESP32
- ESPHome with `api:` enabled
- `api.custom_services: true` so the `pair_<object_id>` service can be registered
- `esp32.framework.type: esp-idf`

## Usage

```yaml
esp32:
  board: esp32-c3-devkitm-1
  framework:
    type: esp-idf

api:
  custom_services: true

external_components:
  - source:
      type: local
      path: /path/to/esphome-lampsmart-hf-light

light:
  - platform: lampsmart_hf_light
    name: Bedroom Lamp
    address: [0xC4, 0xC0, 0xFF, 0xFF]
    group: 0x10
    duration: 2000
    cold_white_color_temperature: 167 mireds
    warm_white_color_temperature: 333 mireds
```

Pairing is exposed as a custom API service named `esphome.<node_name>_pair_<object_id>`. Power-cycle the lamp, then call the service during the pairing window.
