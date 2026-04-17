# Jan Drone nRF24 Link

This project uses `nRF24L01+` as the custom control transport.

The radio path is:

```text
PS5 controller -> controller ESP32 -> nRF24 -> flight-controller ESP32 -> ESP-FC input
```

## Flight-controller ESP32 wiring

Fixed pinout used by the receiver implementation:

- `SCK` -> `GPIO 18`
- `MOSI` -> `GPIO 23`
- `MISO` -> `GPIO 19`
- `CSN` -> `GPIO 16`
- `CE` -> `GPIO 17`
- `VCC` -> stable `3.3V`
- `GND` -> shared ground with the ESP32 and ESC signal ground

Keep the `MPU6050` on:

- `SCL` -> `GPIO 22`
- `SDA` -> `GPIO 21`

Keep motor outputs on:

- motor 1 -> `GPIO 27`
- motor 2 -> `GPIO 26`
- motor 3 -> `GPIO 25`
- motor 4 -> `GPIO 33`

## Controller-side ESP32 wiring

Use the same SPI and control pinout on the controller bridge ESP32:

- `SCK` -> `GPIO 18`
- `MOSI` -> `GPIO 23`
- `MISO` -> `GPIO 19`
- `CSN` -> `GPIO 16`
- `CE` -> `GPIO 17`
- `VCC` -> stable `3.3V`
- `GND` -> ESP32 ground

The PS5 controller must pair to an original `ESP32` that supports Bluetooth Classic.

## Power and stability notes

- `nRF24L01+` modules are sensitive to power noise.
- Put a local decoupling capacitor close to each radio module.
- A `10uF` to `47uF` capacitor across `VCC` and `GND` at the radio is recommended.
- Do not power the radio from a noisy or overloaded `3.3V` rail.
- All grounds must be common.

## Packet format

The shared packet lives in [JanDroneNrf24Protocol.h](C:\Users\janve\Documents\Arduino\Stolen%20FC%20from%20online\esp-fc\include\JanDroneNrf24Protocol.h).

Fields:

- `magic`
- `version`
- `flags`
- `sequence`
- `txMillis`
- `channels[7]`
- `crc`

Channel order:

- `0` roll
- `1` pitch
- `2` throttle
- `3` yaw
- `4` arm
- `5` angle/acro
- `6` buzzer

Flags:

- `FLAG_ARM`
- `FLAG_ANGLE`
- `FLAG_BUZZER`
- `FLAG_FAILSAFE`

Timeout policy:

- frame-loss threshold: `120 ms`
- failsafe threshold: `350 ms`

The receiver disarms through `ESP-FC` failsafe handling when packets stop or when `FLAG_FAILSAFE` is set.
