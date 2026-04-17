# ESP-FC Drone Bring-Up Master Plan

## Summary

Use `ESP-FC` in your fork as the only flight-controller infrastructure. Treat the existing Arduino firmware in `Jan drone fc/drone_flight_controller` as reference material only for wiring, motor order, safety ideas, and known-good assumptions.

Default hardware assumptions for the whole plan:
- Flight controller MCU: original `ESP32`
- Controller bridge MCU: separate original `ESP32` with Bluetooth Classic support
- Radio link: `2x nRF24L01+`, one on the flight controller and one on the controller bridge
- IMU: `MPU6050` on `I2C`, address `0x68`
- I2C pins: `SCL 22`, `SDA 21`
- Motor outputs: `27 / 26 / 25 / 33`
- Frame: `Quad X`
- ESC protocol: `DShot300`
- Motor positions: `1 left-back`, `2 left-front`, `3 right-front`, `4 right-back`
- Motor spin intent: `1 CW`, `2 CCW`, `3 CW`, `4 CCW`
- First control milestone: stable bench bring-up first, then PS5 over `nRF24`

Tooling defaults:
- Configuration UI: `Betaflight App` PWA at `app.betaflight.com`
- Flashing tool: `esptool-js` web flasher or `PlatformIO/esptool`
- CLI location: `Betaflight App -> CLI`

## Implementation Plan

### 1. Repository and project foundation
- Standardize on your fork as the source of truth: `origin = JanBlaBla/esp-fc`, `upstream = rtlopez/esp-fc`.
- Create a dedicated branch strategy for your drone bring-up work.
- Add a project README in the fork that captures your exact hardware map, board variant, ESCs, IMU, controller plan, and safety rules.
- Record the current custom firmware's proven assumptions so nothing important is lost during migration.
- Decide one build flow for the fork and document it end-to-end on your machine: `PlatformIO`, flashing method, Betaflight App browser requirements, fallback desktop version, serial driver expectations.
- Add a local bring-up checklist document for "props off" bench work only.

### 2. Hardware definition and wiring freeze
- Freeze the exact ESP32 board model you are using.
- Verify boot-safe usage of all chosen GPIOs, especially the four motor pins.
- Verify voltage domains for `MPU6050`, ESC signal lines, receiver/control path, buzzer, and battery sense if added later.
- Confirm whether the IMU interrupt pin will be used or ignored for the first milestone.
- Confirm whether battery voltage monitoring will be included in the first integration or deferred.
- Confirm whether a buzzer and status LED will be wired now or later.
- Draw one final wiring map for the flight controller.
- Draw one final motor numbering diagram that matches the firmware, frame layout, and prop direction.
- Label all connectors and wires physically to reduce setup mistakes.

### 3. ESP-FC baseline configuration
- Build the unmodified fork successfully on your machine before changing behavior.
- Flash stock `ESP-FC` once to confirm the board is detectable and recoverable.
- Connect to the Betaflight App PWA and confirm CLI access.
- Capture the stock `dump`, `diff`, and `get pin` outputs for your board as a baseline.
- Set `pin_i2c_scl = 22` and `pin_i2c_sda = 21` if not already correct.
- Remap motor outputs to your wiring on `pin_output_0..3`.
- Set `output_motor_protocol = DSHOT300`.
- Set the correct receiver mode for the first milestone.
- Disable unused features during bring-up to reduce variables.
- Save a known-good initial CLI profile for your board in the repo.

### 4. Sensor bring-up
- Confirm the `MPU6050` is detected consistently on boot.
- Verify board orientation in the Configurator model view.
- Set `board alignment` or `sensor alignment` until frame movement and model movement match exactly.
- Calibrate accelerometer on a known level surface.
- Verify roll, pitch, and yaw signs manually by tilting the frame.
- Check gyro noise and loop timing at rest.
- Confirm the IMU remains stable when powered from the full flight power system, not just USB.
- Document any offsets, trim bias, or mounting quirks.

### 5. Motor and ESC bring-up
- Confirm all four ESCs initialize reliably with `DShot300`.
- Confirm motor output order matches the Quad X diagram in `ESP-FC`.
- Confirm each motor spins when selected in motor test mode.
- Confirm each motor spins in the intended direction.
- Reverse motor direction at ESC level or wiring level as needed.
- Confirm idle behavior and motor stop behavior.
- Verify there is no desync, twitching, or random startup behavior.
- Check whether `DShot300` is fully stable on your chosen pins; if not, fallback plan is `DShot150`.
- Save the validated motor mapping and direction state in the repo docs.

### 6. Receiver and control-input architecture
- Treat `ESP-FC` as requiring receiver-style input, not raw gamepad packets.
- Use a dedicated controller bridge ESP32 for PS5 input.
- Use `nRF24L01+` only as the transport between controller bridge and flight controller.
- Convert the `nRF24` payload into a receiver-style input stream inside `ESP-FC`.
- Keep native PS5 Bluetooth directly inside the FC process out of the first airworthy milestone.
- Define one canonical stick mapping for all control paths:
  - Throttle
  - Roll
  - Pitch
  - Yaw
  - Arm
  - Mode switch
  - Failsafe behavior
- Define one canonical channel map that matches `ESP-FC` expectations.
- Define deadband, expo, rates, and arming conditions once and reuse them across all controller adapters.
- Document controller reconnect behavior and what the FC must do on packet loss.

### 7. PS5 integration plan
- Confirm you are using an original `ESP32`, not `ESP32-S3/C3/C6/H2`, for any native DualSense Bluetooth Classic work.
- Use a separate ESP32 bridge.
- Keep separation of concerns:
  - FC runs `ESP-FC`
  - bridge handles PS5 pairing and radio transmission
- Define exact button mapping:
  - arm/disarm
  - angle/acro mode
  - buzzer
  - emergency disarm
  - optional tuning shortcuts
- Define reconnect and startup order for controller power-up.
- Define what happens if the controller disconnects while armed.
- Define whether Bluetooth is only for bench testing at first or allowed for hover tests.

### 8. Safety system definition
- Freeze a preflight checklist for every powered test.
- Enforce props-off rules for all setup, mapping, and bench tuning tasks.
- Define arming requirements clearly:
  - valid IMU
  - valid receiver link
  - low throttle
  - mode switch in safe state
- Define disarm triggers clearly:
  - receiver timeout
  - explicit switch
  - crash or angle threshold if implemented
  - USB-only bench mode if desired
- Define a simple Stage 1 and Stage 2 failsafe policy consistent with what `ESP-FC` supports.
- Verify failsafe behavior on link loss before any prop-on test.
- Add a bench-safe emergency shutdown procedure that is practiced before flight attempts.

### 9. Bench validation milestone
- Achieve repeatable power-up with IMU detection and clean boot.
- Achieve repeatable Configurator connection over USB.
- Achieve repeatable receiver input visibility in the Receiver tab.
- Confirm channel directions and midpoint values.
- Confirm stick endpoints and switch ranges.
- Confirm arm switch logic works only under safe conditions.
- Confirm motor tab tests match physical motor order.
- Perform props-off tilt response checks and confirm correction goes in the right direction.
- Confirm there is no runaway correction from wrong axis signs or wrong board alignment.
- Capture one reference CLI `diff all` after this milestone.

### 10. First prop-on restrained test
- Only proceed after all props-off validation passes.
- Use low-risk props and a restrained test setup or open safe area.
- Start in the simplest stabilized mode available.
- Verify idle, spool-up smoothness, and immediate disarm behavior.
- Abort immediately on any yaw spin, flip tendency, or runaway correction.
- After each short test, log observed symptoms and exact configuration state.
- Change one variable at a time between tests.

### 11. Initial tuning plan
- Start with stock `ESP-FC` rates and conservative PID settings.
- Tune only after alignment, motor order, direction, and receiver mapping are correct.
- Tune in this order:
  - hover stability
  - roll/pitch oscillation control
  - yaw authority
  - filter refinement
  - rates/expo feel
- Keep one saved "known flyable" config before every tuning round.
- Use short hover hops first, not full flights.
- If blackbox is available, enable it early for tuning data.
- Keep a written tuning log with exact date, config diff, symptom, and result.

### 12. Logging, diagnostics, and tooling
- Enable blackbox logging if the board and storage path support it.
- If onboard flash is insufficient, define a serial logging path.
- Capture boot logs, receiver status, CPU load, and loop rate.
- Track which CLI values are non-default for your build.
- Add a repo folder for saved `diff all`, wiring notes, and test logs.
- Add a one-command or one-doc flashing flow so recovery is fast after bad configs.
- Document how to restore a known-good config from scratch.

### 13. Power system and airframe integration
- Verify FC power supply stability under motor load.
- Check ESC power and signal grounding layout.
- Check IMU vibration isolation and mounting stiffness.
- Check CG and note whether pitch trim is needed because of battery placement.
- Check prop orientation and motor numbering on the real frame.
- Check frame resonance risks and loose hardware before tuning.
- Add battery voltage sensing if you want low-voltage protection and telemetry later.
- Add buzzer and status LED if you want easier field debugging.

### 14. Robustness work after first stable hover
- Improve receiver-loss handling.
- Improve arming-state visibility.
- Add better startup diagnostics.
- Add battery monitoring and low-voltage behavior.
- Add buzzer behaviors for armed, failsafe, and low battery.
- Add saved configuration snapshots per test phase.
- Evaluate whether native PS5 support on the FC is worth the complexity once a bridge path already works.
- Evaluate whether `CRSF/ELRS` becomes a better long-term radio path than PS5 for reliable flight.
- Evaluate GPS, WiFi config, ESP-NOW, and telemetry only after the base quad is stable.

### 15. Documentation and operating discipline
- Maintain one authoritative hardware map in the fork.
- Maintain one authoritative CLI setup script or setup document.
- Maintain one authoritative controller mapping document.
- Maintain one preflight and postflight checklist.
- Maintain one incident log for every crash, brownout, desync, or unexplained behavior.
- Record every tested firmware/config version before flight.
- Keep recovery instructions for reflashing and restoring a stable profile.

## Interfaces and Public Configuration

Important interfaces the implementation should standardize:
- `ESP-FC` CLI configuration for pin mapping, motor protocol, receiver mode, and feature toggles
- One saved baseline `diff all` for your board
- One saved "known flyable" config snapshot
- One documented `nRF24` packet format and pinout
- One documented controller-to-channel mapping for the PS5 path
- One documented safety policy for arm, disarm, and failsafe

Public behavior to freeze early:
- Motor numbering and physical placement
- Motor direction convention
- Stick mapping and channel map
- Receiver mode used for the first milestone
- Flight mode switch mapping
- Failsafe timeout and disarm behavior

## Test Plan

Bench tests required before prop-on:
- FC boots reliably from USB and flight battery.
- Configurator connects reliably.
- `MPU6050` is detected every boot.
- Model orientation matches frame movement.
- Roll, pitch, and yaw signs are correct.
- Receiver tab shows correct channel movement.
- Endpoints, midpoint, and deadband are correct.
- Arming is blocked when throttle is high.
- Arming is blocked when receiver link is absent.
- Motor outputs map to the correct physical motors.
- Motor directions are correct.
- Props-off tilt correction pushes back toward level, not further into the tilt.
- Failsafe disarms on link loss.

Flight-adjacent tests after prop-on:
- Low-throttle spool-up is smooth.
- Short restrained lift test does not yaw-spin or tip instantly.
- Hover test is controllable in stabilized mode.
- Re-arm after disarm behaves consistently.
- Receiver reconnect behavior is predictable and safe.
- Tuning changes are reversible and documented.

## Assumptions and Defaults

- Main line of development is your `ESP-FC` fork only.
- The custom Arduino firmware is reference-only and will not be further developed unless a specific gap in `ESP-FC` forces it.
- First successful milestone is a safe, bench-validated `ESP-FC` setup on your current wiring.
- PS5 support runs through a separate ESP32 controller bridge and `nRF24` transport.
- Receiver-style input compatibility is preferred over bespoke direct gamepad control inside the FC.
- `DShot300` is the default protocol unless signal reliability forces a fallback.
- Betaflight App PWA is the primary setup UI, with legacy desktop only as fallback.
