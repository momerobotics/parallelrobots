# Development Log

This document is a short, GitHub-friendly summary of the raw research notes and project files in this folder.

## 2025 - Research and Concept Phase

### October 2025

- The project direction was defined around rapid prototyping and learning through a working physical system rather than abstract specification alone.
- The main focus became a bilateral haptic connection in which movement and resistance can be perceived mutually.
- Visits to OORI and related institutional contexts reinforced the idea that small-scale, sensitive, therapist-adjacent systems may be more relevant than large industrial-style robotics.

### November 2025

- The rehabilitation context became clearer.
- The project started to take shape as a research platform rather than a finished device.

## 2026 - Hardware and Software Development

### January 2026

- Component sourcing and technical selection began.
- Main components included:
  - `ESP32`-based 20A FOC board
  - `BLDC` motors
  - programming interface, jumpers, and connectors
  - external power supply
- Early research focused on `SimpleFOC`, `ESP32`, gate driver mapping, and current sensing options.

### February 1-2, 2026

- The fixed board `GPIO` mapping was identified.
- The first stable open-loop motor spin was successful.
- Key lesson: the gate driver pins on this board are not freely assignable, so correct pin mapping is critical.

Validated open-loop configuration:

| Function | Pin / value |
| --- | --- |
| Phase A | `GPIO16` |
| Phase B | `GPIO17` |
| Phase C | `GPIO5` |
| Enable | `GPIO4` |
| Power supply | `24V` test environment |
| PWM frequency | `40 kHz` |

### February 3, 2026

- The `PWM` and `Enable` mapping was fully verified.
- The board's current sensing capabilities were also documented.
- `ESP-NOW` communication setup began, including recording the MAC addresses of both boards.

Recorded current sensing data:

| Item | Value |
| --- | --- |
| Type | inline current sensing |
| Shunt | `0.01 Ohm` |
| Gain | `50` |
| Phase A ADC | `GPIO39` |
| Phase B ADC | `GPIO36` |

### February 4-12, 2026

- Iterative experiments continued around motors, wiring, and stability.
- ESP32 core and library compatibility issues were addressed.
- The fundamentals of closed-loop operation were checked in a one-motor / one-controller setup.

### February 13, 2026

- The first stable bilateral mirror system was achieved.
- The two ESP32 boards exchanged angle data in real time over `ESP-NOW`.
- The system produced a mutual physical resistance sensation.
- The first working version was published to GitHub.

## What the Current Sketch Implements

The current `v2` code:

- reads the local motor angle
- sends it to the other board
- receives the remote angle
- generates a spring-like torque from the angle difference
- falls back to zero torque if the link is lost

This is not a rigid position-following system. It is a haptic coupling strategy designed to create a mutually perceivable physical connection.

## Important Note

The research notes also describe current sensing and additional closed-loop possibilities at the platform level, but the current `2026_02_13_parallel_mirror_espnow_v2.ino` sketch primarily documents a sensor-based `SimpleFOC` and `ESP-NOW` mirror behavior.
