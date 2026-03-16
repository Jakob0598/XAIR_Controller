# XAIR Controller (ESP)

Hardware controller for **Behringer X-Air digital mixers** built on an
**ESP microcontroller**.

The project provides a standalone controller with physical controls
(encoders, buttons, faders) and a graphical display to control mixer
parameters over **OSC via WiFi**.

## Features

-   Control Behringer **X-Air mixers**
-   Communication via **OSC**
-   **Rotary encoders, buttons and faders**
-   **Graphical EQ display**
-   **OLED / display interface**
-   **WiFi connection**
-   Integrated **WebUI for configuration**
-   **Battery management**
-   Modular architecture

## Hardware

Designed for ESP-based hardware (e.g. ESP32).

Typical components used:

-   ESP32
-   Rotary encoders
-   Buttons
-   Faders
-   I2C displays
-   LEDs
-   Battery / power management

*(Hardware layout depends on your specific controller build.)*

## Software Architecture

The project is split into several modules:

    src/
     ├── main.cpp                # Main firmware entry
     ├── network_manager         # Network communication
     ├── wifi_manager            # WiFi handling
     ├── osc_manager             # OSC communication
     ├── osc_dispatcher          # OSC routing
     ├── xair_parser             # X-Air message parsing
     ├── mixer_state             # Local mixer state
     ├── display_manager         # Main display handling
     ├── mini_display_manager    # Small display support
     ├── eq_plot_engine          # EQ visualization
     ├── encoder / buttons       # Hardware input handling
     ├── fader                   # Fader input
     ├── led_manager             # LED control
     ├── batterie_manager        # Battery monitoring
     ├── settings                # Device settings
     └── webui                   # Web configuration interface

## Network

The controller connects to the mixer via **WiFi** and communicates using
**OSC messages**, which are used by Behringer X-Air mixers.

## Build

The firmware is intended to be built for an **ESP microcontroller**
using a typical embedded toolchain such as:

-   PlatformIO
-   Arduino framework for ESP32

Example (PlatformIO):

    pio run
    pio upload

## Configuration

Configuration options can be adjusted in:

    src/config.h

Typical settings include:

-   WiFi configuration
-   Mixer IP address
-   Hardware settings

## Status

Work in progress -- functionality and hardware support may change.

## License

MIT License
