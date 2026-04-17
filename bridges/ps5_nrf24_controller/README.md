# PS5 nRF24 Controller Bridge

This sketch runs on a separate original `ESP32`.

It:

- pairs with the PS5 controller using `Bluepad32`
- maps the controller to RC-style channels
- sends those channels over `nRF24L01+`

## Required install

Use the desktop Arduino workflow recommended by Bluepad32 for Arduino on ESP32:

- install the `ESP32 + Bluepad32` board package
- install the `RF24` library
- open [ps5_nrf24_controller.ino](C:\Users\janve\Documents\Arduino\Stolen%20FC%20from%20online\esp-fc\bridges\ps5_nrf24_controller\ps5_nrf24_controller.ino)
- build and flash it to a separate original `ESP32`

Bluepad32 docs:

- [Arduino + ESP32 board](https://bluepad32.readthedocs.io/en/latest/plat_arduino/)
- [Supported gamepads](https://bluepad32.readthedocs.io/en/stable/supported_gamepads/)
- [FAQ about Bluetooth Classic support](https://bluepad32.readthedocs.io/en/stable/FAQ/)

RF24 docs:

- [RF24 GitHub](https://github.com/nRF24/RF24)
- [nRF24 documentation](https://nrf24.github.io/.github/)

## Wiring

Use the same radio pinout documented in [jan-drone-nrf24.md](C:\Users\janve\Documents\Arduino\Stolen%20FC%20from%20online\esp-fc\docs\jan-drone-nrf24.md):

- `SCK` -> `GPIO 18`
- `MOSI` -> `GPIO 23`
- `MISO` -> `GPIO 19`
- `CSN` -> `GPIO 16`
- `CE` -> `GPIO 17`
- `VCC` -> clean `3.3V`
- `GND` -> `GND`

Add local decoupling close to the radio module.

## Current mapping

- left stick `X` -> yaw
- left stick `Y` -> throttle
- right stick `X` -> roll
- right stick `Y` -> pitch
- `R1` -> arm
- `L1` -> angle mode
- `Circle / B` -> buzzer

The sketch sends a failsafe packet when no controller is connected.
