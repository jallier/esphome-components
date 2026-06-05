# Crow Alarm Panel - ESPHome Custom Component

ESPHome custom component for integrating Crow/Arrowhead alarm panels with Home Assistant.

## Features

- **Zone Monitoring**: Binary sensors for zone activity and bypass status
- **Armed State**: Text sensor with arming/disarming states
- **Full Alarm Control Panel Entity**: Expose a Home Assistant alarm panel while keeping button/switch entities
- **Keypad Bus Logging**: `on_message` trigger exposes raw bus frames
- **Output State**: Switch entities reflect output states (read-only today)
- **Keypress/Arm/Disarm Buttons**: Sends keypad keypress sequences to arm away/stay or disarm
- **Interrupt-Driven**: ISR-based clocked serial decoding

## Credits

This repo builds on the work done by [jesserockz](https://github.com/jesserockz/esphome-components/tree/main/components/crow_alarm_panel). 
I have added the transmission code, but the bulk of the work is thanks to them.

## Installation

### Method 1: External Components (Recommended)

See [here](https://esphome.io/components/external_components/) for the full reference on external components

Add to your ESPHome YAML configuration:

```yaml
external_components:
  - source: github://yourusername/esphome-crow-alarm@main
    components: [ crow_alarm_panel ]
```

### Method 2: Local Components

1. Clone this repository:
   ```bash
   cd /config  # Your ESPHome config directory
   git clone https://github.com/yourusername/esphome-crow-alarm.git
   ```

2. Reference in your YAML:
   ```yaml
   external_components:
     - source:
         type: local
         path: esphome-crow-alarm
       components: [ crow_alarm_panel ]
   ```

## Configuration

### Basic Setup

```yaml
crow_alarm_panel:
  id: alarm
  clock_pin: 18
  data_pin: 19
  address: 0  # Your keypad address

  keypads:               # Optional: label other keypads for logging
    - name: "Main Keypad"
      address: 0

  # Optional: log every bus message
  on_message:
    - logger.log:
        format: "%02x -  %s"
        args: 
        - "type"
        - "format_hex_pretty(data).c_str()"

# Zones
binary_sensor:
  - platform: crow_alarm_panel
    type: zone
    zone: 1
    name: "Front Door"

  - platform: crow_alarm_panel
    type: bypass
    zone: 1
    name: "Front Door Bypassed"

# Text sensor for armed state
text_sensor:
  - platform: crow_alarm_panel
    type: armed_state
    name: "Alarm Status"

# Full alarm control panel entity (optional)
alarm_control_panel:
  - platform: crow_alarm_panel
    name: "Crow Alarm"
    code: !secret alarm_code        # Optional: default disarm code
    requires_code_to_arm: false     # Optional: set true if your panel needs code to arm
# Buttons, switches, and sensors from above can still be used alongside the full panel entity.

# Output state (read-only; write not implemented)
switch:
  - platform: crow_alarm_panel
    type: output
    output: 1
    name: "Siren (read only)"

# Buttons for keypad-driven actions
button:
  - platform: crow_alarm_panel
    type: arm_away
    name: "Arm Away"

  - platform: crow_alarm_panel
    type: arm_stay
    name: "Arm Stay"

  - platform: crow_alarm_panel
    type: disarm
    name: "Disarm"
    code: !secret alarm_code
```

## Hardware Setup

### Wiring

Connect your ESP32 to the alarm panel keypad bus:

- **Clock Pin (GPIO18)**: Connect to alarm panel clock line through a 5v <-> 3.3v level shifter
- **Data Pin (GPIO19)**: Connect to alarm panel data line through the level shifter
- **Ground**: Common ground between ESP32 and alarm panel
- **Power**: The arrowhead esl panel uses 13v for its power line. You can use a buck converter to step this down to 5v to make it useable by the esp32.

### Recommended Hardware

- ESP32 development board
- Bidirectional Logic level converter
- Buck converter

## Known Limitations

- Output switches are read-only; `set_output` is not implemented yet.
- Setting the address as a different value to the keypad causes the keypad to reset. Unsure why at this point.

## Protocol

This component decodes the proprietary Crow alarm panel serial protocol:

- **Communication Type**: Clock-synchronous serial
- **Bit Order**: LSB-first transmission
- **Frame Markers**: 0x7E boundary bytes
- **Interrupt-Driven**: GPIO interrupt on clock falling edge
- **Message Types**: Zone state, armed state, keypresses, outputs, time, memory events

### Supported Message Types

| Type | Description |
|------|-------------|
| 0x11 | Armed state (armed_away, arming, disarmed, pending) |
| 0x12 | Zone state (triggered, alarmed, bypassed) |
| 0x14 | Keypad command (beep commands) |
| 0x15 | Keypad state (normal, installer, programming) |
| 0x50 | Output state |
| 0x54 | Current time/date |
| 0xA1 | Keypress from physical keypad |
| 0xD2 | Memory cleared |

## Development

### Local Development Setup

Follow the esphome development guide to set up a local environment. You can use this to test that your code compiles.
I then set up the custom component on my regular esphome install via git and pulled it in that to test on a live device.

## Troubleshooting

### Component Not Found

If ESPHome can't find the component:
- Check the `external_components` path is correct
- Verify the component name is `crow_alarm_panel`
- Try adding `refresh: 0s` to always fetch latest version

### No Data Received

If no messages are received from the alarm panel:
- Verify wiring (clock and data pins)
- Check ground connection
- Ensure keypad address is correct
