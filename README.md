[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20809345.svg)](https://doi.org/10.5281/zenodo.20809345)

# Parallel Mirror Steer-by-Wire# Parallel Mirror Steer-by-Wire

An open-source research platform for bilateral embodied human–robot interaction.

Developed at the MOME Robotics Studio, this repository documents an experimental steer-by-wire system that enables two physically separated BLDC motors to exchange motion through real-time wireless communication and local torque control.

The platform supports research into embodied interaction, haptic reciprocity, adaptive robotics, and Research through Design methodologies.

![ESP32 FOC board overview](docs/board-overview.png)

## Abstract

This repository documents a research prototype for a wireless bilateral motor interface. Two BLDC motor nodes exchange angular position data over `ESP-NOW` and generate a mutual spring-like resistance through local torque control. Rather than implementing rigid master-slave tracking, the system explores a softer form of steer-by-wire coupling in which motion on one side is physically felt on the other.

The project sits at the intersection of rapid prototyping, interaction design, and rehabilitation-oriented haptic research. Its main contribution is a working proof of concept for a lightweight, mechanically unlinked, mutually perceivable motor interaction using low-cost embedded hardware.

## Research Context

The prototype was developed as part of a broader investigation into bilateral physical feedback systems. A key motivation was to build an experimental platform that can be iterated quickly and used to study how resistance, compliance, and reciprocity can be designed into small-scale motorized interfaces.

Instead of treating the system as a finished device, this repository presents it as a research instrument: a functional platform for testing haptic behavior, control strategies, and possible future applications in assistive or rehabilitation contexts.

## System Overview

Each node in the system:

- runs a local `FOC` control loop on an `ESP32`
- reads shaft position from an `AS5048` magnetic angle sensor
- sends the measured angle to the other node over `ESP-NOW`
- computes the difference between local and remote angle
- converts that difference into a softened spring response
- falls back to zero torque if communication is lost

This produces a bilateral coupling effect that feels like shared resistance rather than exact positional locking.

## Current Prototype Status

- validated `ESP32 FOC` board pin mapping
- stable open-loop motor test completed
- working `ESP-NOW` communication between two boards
- bilateral mirror behavior running in the current `v2` sketch
- suitable as a research prototype, not yet as a deployable device

## Hardware Platform

### Main Components

- `2x` ESP32-based 20A FOC driver board
- `2x` BLDC motor
- `2x` `AS5048` magnetic angle sensor
- external power supply
- wiring, connectors, and programming interface

### Validated Board Information

| Item | Value / mapping | Notes |
| --- | --- | --- |
| MCU | `ESP32-WROOM-32E` | onboard module |
| Gate driver | `IR2104` | 3 half-bridge drivers |
| MOSFET stage | `30V / 30A` | board-level power stage |
| Current sense | `INA240A2` | available on the board, not actively used in the current sketch |
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

The initial open-loop hardware validation was performed in a `24V` setup, while the current sketch uses `SUPPLY_VOLTAGE = 12.0f`. For first tests, low voltage and torque limits are strongly recommended.

## Control Strategy

The current implementation uses a simple but effective haptic coupling model:

1. local shaft angle is sampled continuously
2. angle data is transmitted to the peer node at about `500 Hz`
3. the angular difference is measured locally
4. a deadband removes small offsets around zero
5. the remaining error is shaped through a nonlinear spring curve
6. the resulting value is applied in torque-voltage mode

Key runtime parameters in the current sketch:

| Parameter | Value | Meaning |
| --- | --- | --- |
| `ESPNOW_CHANNEL` | `1` | fixed communication channel |
| `SEND_PERIOD_US` | `2000` | about `500 Hz` send rate |
| `TIMEOUT_MS` | `250` | link timeout threshold |
| `VOLTAGE_LIMIT` | `7.0f` | maximum control voltage |
| `SPRING_K` | `2.0f` | spring strength |
| `DEAD_BAND_RAD` | `0.08f` | no-resistance zone around zero |
| `SOFTEN_EXP` | `1.4f` | nonlinear response shaping |
| `MIRROR_SIGN` | `+1` | direction inversion if needed |

## Software

Main sketch:

- `2026_02_13_parallel_mirror_espnow_v2/2026_02_13_parallel_mirror_espnow_v2.ino`

Main libraries:

- `SimpleFOC`
- `esp_now`
- `WiFi`
- `esp_wifi`

## Setup

### Requirements

- `Arduino IDE` or `PlatformIO`
- installed `ESP32` board support
- installed `SimpleFOC` library

### Wiring

- connect the BLDC motor to the `A / B / C` outputs
- connect the `AS5048` sensor to the `SPI` pins defined in the sketch
- verify power and ground wiring carefully before startup

### MAC Address Configuration

Each board must target the other board's MAC address. The current sketch contains both addresses near the top:

```cpp
// Running on Board A: peer = Board B
uint8_t peerAddress[] = { 0x88, 0x57, 0x21, 0xBC, 0x7C, 0x00 };
// Running on Board B: peer = Board A
// uint8_t peerAddress[] = { 0x88, 0x57, 0x21, 0xBB, 0xF3, 0x20 };
```

### First Power-Up Checklist

- start with a low `VOLTAGE_LIMIT`
- confirm sensor initialization is stable
- if the direction is reversed, set `MIRROR_SIGN = -1`
- verify that link timeout correctly drops torque to zero

## Parameter Tuning

The most useful parameters for shaping the feel of the interaction are:

- `VOLTAGE_LIMIT` for overall force level
- `SPRING_K` for coupling strength
- `DEAD_BAND_RAD` for center free-play
- `SOFTEN_EXP` for softening the near-zero response
- `SEND_PERIOD_US` for communication update frequency
- `TIMEOUT_MS` for disengagement speed after link loss

## Prototype Limitations

- the current implementation is still a research prototype
- current sensing is documented at the board level but not yet central to the active control strategy
- there is no full fault management, emergency stop layer, or production safety envelope
- mechanical integration and long-duration reliability testing are still future work

## Safety Notes

- this is a high-current motor controller board, so initial tests should always be mechanically secured
- incorrect phase order, sensor wiring, or overly high `VOLTAGE_LIMIT` can cause vibration, heating, or sudden motion
- do not treat the current prototype as a clinically validated or deployment-ready system

## Repository Contents

- main prototype sketch: `2026_02_13_parallel_mirror_espnow_v2/`
- development summary: [`docs/development-log.md`](docs/development-log.md)
- board reference image: `docs/board-overview.png`
- raw project notes: `KUTATÁSI_NAPLÓ.docx`
- detailed project log: `2026_02_15_Steer By Wire Naplo 2025 2026.docx`

## Future Directions

- integrate current sensing more directly into torque control
- add zero-point calibration and startup routines
- improve filtering and smoothing
- add optional diagnostics and debug tooling
- continue mechanical integration and user testing

## Project Status

This repository documents a working proof of concept. At its current stage, the project demonstrates that a bilateral, wireless, mutually perceivable motor mirror connection can be implemented in an `ESP32 + SimpleFOC` environment using a lightweight embedded architecture.
