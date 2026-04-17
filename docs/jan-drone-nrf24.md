# Jan Drone Temporary Wireless Link

This project currently uses `ESP-NOW` as the temporary control transport during configuration and bench bring-up.

The radio path is:

```text
PS5 controller -> controller ESP32 -> ESP-NOW -> flight-controller ESP32 -> ESP-FC input
```

`nRF24L01+` remains a later migration path once the radio modules arrive and the base quad is already stable.

## Flight-controller ESP32

For the current temporary link, the drone ESP32 uses the built-in `ESP-FC` ESP-NOW receiver path. No external radio wiring is required for the control transport.

Keep the `MPU6050` on:

- `SCL` -> `GPIO 22`
- `SDA` -> `GPIO 21`

Keep motor outputs on:

- motor 1 -> `GPIO 33`
- motor 2 -> `GPIO 25`
- motor 3 -> `GPIO 26`
- motor 4 -> `GPIO 27`

These motor pins match the validated bench preset in `jan-drone-base.cli`, not the older draft assumption from the first planning pass.

## Controller-side ESP32

The PS5 controller must pair to an original `ESP32` that supports Bluetooth Classic.

Reference material for the controller side is kept in:

- workspace prototype folder: `Input bridge/`
- maintained FC bridge sketch: `esp-fc/bridges/ps5_nrf24_controller/`

## Receiver frame

The bridge must send the built-in `espnow-rclink` RC frame that `ESP-FC` already expects over `InputEspNow`.

Channel order:

- `0` roll
- `1` pitch
- `2` throttle
- `3` yaw
- `4` arm
- `5` angle/acro
- `6` buzzer
- `7` reserved

Current PS5 to channel mapping used by the maintained bridge sketch:

- left stick `X` -> channel `3` yaw
- left stick `Y` -> channel `2` throttle with safe centered-stick remap
- right stick `X` -> channel `0` roll
- right stick `Y` -> channel `1` pitch
- `R1` -> channel `4` arm
- `L1` -> channel `5` angle/acro
- `Circle` -> channel `6` buzzer
- channel `7` reserved and held low

The `Input bridge/Receive_Data/Receive_Data.ino` sketch is the raw input reference for these controls. The conversion to RC-style PWM values happens in `bridges/ps5_nrf24_controller/ps5_nrf24_controller.ino`.

## Safe throttle behavior

Because the PS5 left stick is self-centering:

- centered stick -> minimum throttle
- downward travel -> minimum throttle
- only upward travel from center raises throttle

This is required so that controller connect and controller reconnect never present as mid-throttle in the Receiver tab.

## Later nRF24 migration

Once the `nRF24L01+` modules arrive, they can be evaluated as a separate transport option. That migration is not part of the temporary ESP-NOW bring-up path.
