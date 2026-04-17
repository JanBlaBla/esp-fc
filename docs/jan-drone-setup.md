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

- enables `FEATURE_RX_SPI` for the nRF24 receiver
- keeps `MPU6050` on `GPIO 21/22`
- maps motors to `27 / 26 / 25 / 33`
- disables serial and buzzer pin conflicts with those motors
- sets `DShot300`
- sets `ARM`, `AIRMODE`, `ANGLE`, and `BUZZER` switch ranges

## First bench checks

Run these before connecting props:

1. confirm the Setup tab model moves with the frame
2. calibrate accelerometer on a level surface
3. open the Motors tab and confirm motor order
4. verify motor direction physically
5. power the controller bridge and verify channels move in the Receiver tab
6. verify link-loss causes failsafe and disarm
