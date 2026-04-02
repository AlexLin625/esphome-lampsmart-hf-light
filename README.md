# esphome-lampsmart-hf-light

ESPHome external component for HF-protocol BLE lamps that are controlled entirely through non-connectable BLE advertisements.

This component packages the validated HF-protocol BLE light implementation as an ESPHome `external_components` light platform. It supports:

- Pair
- Turn on / turn off
- Brightness
- Cold/warm white color temperature

## Requirements

- ESP32
- `esp32.framework.type: esp-idf`

## Usage

```yaml
esp32:
  board: esp32-c3-devkitm-1
  framework:
    type: esp-idf

external_components:
  - source:
      type: local
    path: /path/to/esphome-lampsmart-hf-light/components

light:
  - platform: lampsmart_hf_light
    name: Bedroom Lamp
    output_id: bedroom_lamp
    address: [0xC4, 0xC0, 0xFF, 0xFF]
    group: 0x0A
    duration: 600
    cold_white_color_temperature: 250 mireds
    warm_white_color_temperature: 500 mireds

button:
  - platform: lampsmart_hf_light
    name: Bedroom Lamp Pair
    lampsmart_hf_light_id: bedroom_lamp
```

Power-cycle the lamp, then press the pairing button entity during the pairing window.

`group` must match the lamp's control group/page ID. The reference `radar-smart-lamp` project uses `0x0A`; pairing uses a special packet and can still succeed even when the control group is wrong.

This platform intentionally disables light transitions and defaults `duration` to `600ms`, because these lamps often ignore follow-up commands while they are still reacting to a previous packet.

By default the HA color temperature range is exposed as about `4000K..2000K` (`250..500` mireds), and the protocol value is mapped to `0..140`.
