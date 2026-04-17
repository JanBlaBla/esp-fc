# Jan Drone Setup

## Required tools

- Chromium-based desktop browser for [Betaflight App](https://app.betaflight.com)
- `PlatformIO` for local builds
- `esptool-js` or `PlatformIO/esptool` for flashing

Use the Betaflight App PWA at [app.betaflight.com](https://app.betaflight.com) for normal configuration.

Use a legacy desktop Betaflight Configurator release only as fallback if WebSerial / WebUSB does not work correctly on your machine.

## Flashing

The current merged ESP32 image is built at:

- [firmware_0x00.bin](C:\Users\janve\Documents\Arduino\Stolen%20FC%20from%20online\esp-fc\.pio\build\esp32\firmware_0x00.bin)

Flash that image at offset `0x0`.

## Applying the board preset

After flashing:

1. connect the flight-controller ESP32 over USB
2. open [app.betaflight.com](https://app.betaflight.com) in a Chromium-based browser
3. click `Connect`
4. open the `CLI` tab
5. paste the contents of [jan-drone-base.cli](C:\Users\janve\Documents\Arduino\Stolen%20FC%20from%20online\esp-fc\docs\jan-drone-base.cli)
6. let `save` and `reboot` execute

If the PWA cannot connect reliably, use the legacy `10.10.0` desktop release as a fallback for the same CLI flow.

The preset:

- enables `FEATURE_RX_SPI` for the built-in ESP-NOW receiver path
- keeps the validated I2C pins on `GPIO 21/22`
- maps motors to the validated bench order `33 / 25 / 26 / 27`
- disables serial and buzzer pin conflicts with those motors
- sets `DShot300`
- sets `ARM`, `ANGLE`, and `BUZZER` switch ranges

## Controller bridge

Use a separate original `ESP32` as the PS5 controller bridge.

The maintained bridge sketch is:

- [ps5_nrf24_controller.ino](C:\Users\janve\Documents\Arduino\Stolen%20FC%20from%20online\esp-fc\bridges\ps5_nrf24_controller\ps5_nrf24_controller.ino)

The newly added workspace folder:

- `Input bridge/`

should be treated as a controller-input reference and pairing prototype, not as the canonical FC integration point.

Current control mapping expected by the receiver setup:

- left stick `X` -> yaw
- left stick `Y` -> throttle with safe centered-stick remap
- right stick `X` -> roll
- right stick `Y` -> pitch
- `R1` -> arm
- `L1` -> angle mode
- `Circle` -> buzzer

Current temporary wireless path:

```text
PS5 controller -> ground ESP32 over Bluetooth -> ESP-NOW -> drone ESP32 running ESP-FC
```

For this temporary phase, `nRF24` is deferred. The built-in `ESP-FC` ESP-NOW receiver path is the active transport.

Safe throttle rule:

- centered left stick must read near minimum throttle in the Receiver tab
- pulling the stick downward must stay at minimum throttle
- only pushing the stick upward from center should raise throttle

## First bench checks

Run these before connecting props:

1. confirm the Setup tab model moves with the frame
2. calibrate accelerometer on a level surface
3. open the Motors tab and confirm motor order
4. verify motor direction physically
5. power the controller bridge and verify channels move in the Receiver tab
6. verify link-loss causes failsafe and disarm
7. verify `R1`, `L1`, and `Circle` hit the expected AUX channels and mode ranges
8. verify centered throttle reads near `1000`, not `1500`
9. verify pushing the left stick down does not increase throttle
