# Parallel Mirror Steer-by-Wire

Wireless bilateral haptic mirror prototype built with `ESP32 + SimpleFOC + ESP-NOW`.

![ESP32 FOC board overview](docs/board-overview.png)

## Overview

This project connects two BLDC motor units wirelessly. Each side uses an `ESP32`-based FOC controller board to read the local shaft angle, transmit it to the other side over `ESP-NOW`, and generate a spring-like resistive response from the received position.

The goal is to create a steer-by-wire / mirror platform in which the two sides react to each other's movement without a mechanical linkage, while still producing a mutual physical feedback sensation. The project was developed in a design research and rapid prototyping context, with interest in haptic interaction and rehabilitation-oriented applications.

## Current Status

- The high-current `ESP32 FOC` board pin mapping has been validated.
- The motor was successfully tested in open-loop mode.
- `ESP-NOW` communication between the two boards works.
- The current `v2` sketch already produces bilateral mirror behavior.
- The code is a research prototype, not a finished product.

## How It Works

1. Each controller runs its own `FOC` loop locally.
2. The sketch periodically sends the local shaft angle to the other board.
3. The remote angle is compared to the local angle to compute an offset.
4. A deadbanded, nonlinearly softened spring response is applied to that offset.
5. If the wireless link times out, motor torque is set to zero.

The result is not strict position tracking, but a mutual physical coupling that feels like shared resistance.

## Hardware

### Main Components

- `2x` ESP32-based 20A FOC driver board
- `2x` BLDC motor
- `2x` magnetic angle sensor with `AS5048` SPI interface
- external power supply
- wires, connectors, and programming interface

### Validated Board Information

| Item | Value / mapping | Notes |
| --- | --- | --- |
| MCU | `ESP32-WROOM-32E` | onboard module |
| Gate driver | `IR2104` | 3 half-bridge drivers |
| MOSFET stage | `30V / 30A` | board-level power stage |
| Current sense | `INA240A2` | supported by the board, but not actively used in the current sketch |
| Motor output | `A / B / C` | 3-phase BLDC connection |

### Pinout Used by the Current Sketch

| Function | Pin |
| --- | --- |
| `PWM_A` | `GPIO16` |
| `PWM_B` | `GPIO17` |
| `PWM_C` | `GPIO5` |
| `EN_PIN` | `GPIO4` |
| `SPI_SCK` | `GPIO18` |
| `SPI_MISO` | `GPIO19` |
| `SPI_MOSI` | `GPIO23` |
| `SPI_CS` | `GPIO13` |

### Power Note

The open-loop hardware validation was performed in a `24V` setup, but the current sketch uses `SUPPLY_VOLTAGE = 12.0f`. For first power-up, it is safest to start with low voltage and torque limits.

## Software Structure

Current main sketch:

- `2026_02_13_parallel_mirror_espnow_v2/2026_02_13_parallel_mirror_espnow_v2.ino`

### Libraries

- `SimpleFOC`
- `esp_now`
- `WiFi`
- `esp_wifi`

### Important Runtime Parameters

| Parameter | Value | Meaning |
| --- | --- | --- |
| `ESPNOW_CHANNEL` | `1` | fixed communication channel |
| `SEND_PERIOD_US` | `2000` | about `500 Hz` send rate |
| `TIMEOUT_MS` | `250` | link timeout threshold |
| `VOLTAGE_LIMIT` | `7.0f` | maximum control voltage |
| `SPRING_K` | `2.0f` | spring strength |
| `DEAD_BAND_RAD` | `0.08f` | deadband where no resistance is applied |
| `SOFTEN_EXP` | `1.4f` | nonlinear softening curve |
| `MIRROR_SIGN` | `+1` | direction inversion if needed |

## Setup

### 1. Required Environment

- `Arduino IDE` or `PlatformIO`
- installed `ESP32` board support
- installed `SimpleFOC` library

### 2. Hardware Wiring

- Connect the BLDC motor to the board `A / B / C` outputs.
- Connect the `AS5048` sensor to the `SPI` pins defined in the sketch.
- Verify power and ground connections carefully.

### 3. MAC Address Configuration

The sketch contains two board MAC addresses near the top. One board should use one `peerAddress`, and the other board should use the opposite one.

```cpp
// Running on Board A: peer = Board B
uint8_t peerAddress[] = { 0x88, 0x57, 0x21, 0xBC, 0x7C, 0x00 };
// Running on Board B: peer = Board A
// uint8_t peerAddress[] = { 0x88, 0x57, 0x21, 0xBB, 0xF3, 0x20 };
```

### 4. First Power-Up

- Start with a low `VOLTAGE_LIMIT`.
- If the direction is reversed, change `MIRROR_SIGN` to `-1`.
- Confirm that sensor initialization and `FOC` startup are stable.
- If the wireless link fails, the sketch falls back to zero motor torque.

## Tunable Parameters

If the system feels too stiff, noisy, or unstable, these are the most useful parameters to adjust:

- `VOLTAGE_LIMIT`: overall force / torque feel
- `SPRING_K`: strength of mutual resistance
- `DEAD_BAND_RAD`: amount of free play around center
- `SOFTEN_EXP`: softness of the near-zero response
- `SEND_PERIOD_US`: communication update speed
- `TIMEOUT_MS`: how quickly the system disengages after link loss

## Safety Notes

- This is a high-current motor controller board, so first tests should always be done with low limits and mechanically secured hardware.
- Incorrect phase order, sensor wiring, or overly high `VOLTAGE_LIMIT` can cause vibration, heating, or sudden motion.
- The current sketch is a research prototype and does not include full fault handling, emergency stop logic, or industrial-grade protection.

## Future Development

- integrate current sensing into torque control more directly
- add zero-point calibration and startup routines
- improve filtering and smoothing
- add optional diagnostics / debug mode
- continue mechanical integration and user testing

## Source Material in This Folder

- research and project notes: `KUTATÁSI_NAPLÓ.docx`
- detailed steer-by-wire log: `2026_02_15_Steer By Wire Naplo 2025 2026.docx`
- board reference images: `photos of the board/`
- AliExpress board reference PDF

A short Markdown summary of the raw notes is available here:

- [`docs/development-log.md`](docs/development-log.md)

## Project Status

This repository documents a working research prototype. The main result so far is that a bilateral, wireless, mutually perceivable motor mirror connection is feasible in an `ESP32 + SimpleFOC` environment.
