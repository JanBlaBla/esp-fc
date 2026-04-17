# PS5 ESP-NOW Controller Bridge

This sketch runs on a separate original `ESP32`.

It:

- pairs with the PS5 controller using `Bluepad32`
- maps the controller to RC-style channels
- sends those channels over `ESP-NOW`

If you added the workspace folder `Input bridge/`, treat it as the raw-input prototype and button-layout reference. This sketch is the maintained path that stays aligned with the built-in `ESP-FC` ESP-NOW receiver path.

## Required install

Use the desktop Arduino workflow recommended by Bluepad32 for Arduino on ESP32:

- install the `ESP32 + Bluepad32` board package
- use the built-in ESP32 Wi-Fi / ESP-NOW headers from that board package
- open [ps5_nrf24_controller.ino](C:\Users\janve\Documents\Arduino\Stolen%20FC%20from%20online\esp-fc\bridges\ps5_nrf24_controller\ps5_nrf24_controller.ino)
- build and flash it to a separate original `ESP32`

Bluepad32 docs:

- [Arduino + ESP32 board](https://bluepad32.readthedocs.io/en/latest/plat_arduino/)
- [Supported gamepads](https://bluepad32.readthedocs.io/en/stable/supported_gamepads/)
- [FAQ about Bluetooth Classic support](https://bluepad32.readthedocs.io/en/stable/FAQ/)

ESP-NOW docs:

- [Arduino ESP32 ESP-NOW API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/espnow.html)
- [ESP-IDF ESP-NOW API](https://docs.espressif.com/projects/esp-idf/en/v5.3.2/esp32/api-reference/network/esp_now.html)
- [espnow-rclink in this workspace](C:\Users\janve\Documents\Arduino\Stolen%20FC%20from%20online\esp-fc\.pio\libdeps\esp32\espnow-rclink\README.md)

## Transport

This bridge implements the same `espnow-rclink` frame and pairing behavior that the drone-side `ESP-FC` receiver already expects, but it does so directly in the sketch so Arduino IDE does not need an extra `espnow-rclink` library install.

## Current mapping

- left stick `X` -> yaw
- left stick `Y` -> throttle with safe centered-stick remap
- right stick `X` -> roll
- right stick `Y` -> pitch
- `R1` -> arm latch toggle
- `L1` -> angle mode latch toggle
- `Circle / B` -> buzzer
- aux 8 -> held low / reserved

## Safe throttle behavior

The PS5 left stick is self-centering, so centered stick must not mean half throttle.

This sketch therefore maps throttle as:

- centered stick -> minimum throttle
- stick down -> minimum throttle
- stick up from center -> increasing throttle
- full stick up -> maximum throttle

If the controller disconnects, the sketch immediately transmits the safe frame:

- roll `1500`
- pitch `1500`
- throttle `1000`
- yaw `1500`
- arm `1000`
- angle `1000`
- buzzer `1000`
- aux 8 `1000`

The raw controller field order used as reference during bring-up is documented in:

- `Input bridge/README.md`

The sketch continuously transmits a safe RC frame when no controller is connected.

## Button latch behavior

- pressing `R1` toggles arm on/off
- pressing `L1` toggles angle mode on/off
- `Circle / B` remains momentary
- controller disconnect resets the arm and angle latches back to low for safety
