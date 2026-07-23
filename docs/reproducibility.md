# Reproduction Guide

This guide describes the exact tested configuration of the Parallel Mirror
Steer-by-Wire prototype. It separates reproduction of the validated two-node
system from adaptation to replacement hardware.

The system is a supervised research prototype. Complete the staged checks below
before enabling motor drive.

## 1. Technical Lineage

The motor-control foundation is
[Arduino SimpleFOC](https://github.com/simplefoc/Arduino-FOC). The initial
bilateral control concept comes from SimpleFOC's official
[Haptics — Steer by Wire example](https://docs.simplefoc.com/haptics_examples),
which applies opposite torque-voltage requests proportional to the angular
difference between two BLDC motors.

This project distributes that virtual coupling across two independent ESP32
nodes. It adds:

- AS5048A SPI sensing at each motor
- ESP-NOW position and velocity exchange
- node identity and packet protocol versioning
- peer-synchronized startup
- nonlinear spring, damping, integral correction, and progressive end-stop
- measured phase-current and speed foldback
- a software thermal-load estimate
- communication timeout and supervised handoff latching

The project is therefore an adaptation and extension of the SimpleFOC example,
not an unchanged copy of its hardware or firmware.

## 2. Tested Bill of Materials

| Quantity | Component | Tested specification |
| ---: | --- | --- |
| 2 | Makerbase MKS ESP32 FOC Mega | single-motor board, ESP32-WROOM-32E |
| 2 | iPower GM3506 brushless gimbal motor | hollow shaft, `24N/22P`, `11` pole pairs |
| 2 | AS5048A encoder assembly | magnetic absolute encoder, `14-bit` SPI |
| 2 | Protected `3S` LiPo battery | one electrically separate pack per node |
| 2 | USB-A-to-USB-C service cable | required by the tested board revision |
| 1 set | Motor, encoder, battery, and mechanical wiring | secured and strain-relieved |

Motor identification source:
[iPower GM3506 with AS5048A](https://shop.iflight.com/ipower-motor-gm3506-brushless-gimbal-motor-w-as5048a-encoder-pro1155).
Encoder reference:
[ams OSRAM AS5048A datasheet](https://look.ams-osram.com/m/287d7ad97d1ca22e/original/AS5048-DS000298.pdf).

Do not substitute a different GM3506 winding, pole count, encoder interface, or
motor controller without repeating calibration and validation.

## 3. Node Architecture

Each node is electrically self-contained:

```text
protected 3S LiPo
        |
        v
MKS ESP32 FOC Mega ---- A/B/C ---- iPower GM3506
        |
        +---- SPI ---- AS5048A
        |
        +---- ESP-NOW wireless link ---- peer node
```

The two nodes do not share motor power. During standalone operation, disconnect
USB from both boards and power each node from its own protected battery.

## 4. Validated Signal Mapping

| Function | ESP32 pin | Connection |
| --- | ---: | --- |
| Motor PWM A | `GPIO32` | onboard M0 inverter |
| Motor PWM B | `GPIO33` | onboard M0 inverter |
| Motor PWM C | `GPIO25` | onboard M0 inverter |
| Motor enable | `GPIO12` | onboard M0 enable |
| AS5048A SCK | `GPIO18` | encoder SPI clock |
| AS5048A MISO | `GPIO19` | encoder data to ESP32 |
| AS5048A MOSI | `GPIO23` | encoder command data |
| AS5048A CS | `GPIO5` | encoder chip select |
| Phase-current A | `GPIO39` | onboard INA240A2 output |
| Phase-current B | `GPIO36` | onboard INA240A2 output |
| Startup VIN sense | `GPIO13` | onboard divider; ADC2 |

Connect the motor phases to the board's `A / B / C` terminal. Phase order,
encoder direction, and electrical-zero calibration are a matched set; do not
change one without recalibrating.

The repository documents logical signals, not wire-color assumptions. Verify the
physical connector pin order against the actual board and encoder assembly
before applying power.

## 5. Verified Software Environment

The sketches were compiled successfully on 2026-07-23 with:

| Tool | Version / target |
| --- | --- |
| Arduino CLI | `1.4.1` |
| Espressif ESP32 Arduino core | `3.3.10` |
| Simple FOC | `2.4.0` |
| Board target | `ESP32 Dev Module` |
| FQBN | `esp32:esp32:esp32` |

Equivalent Arduino IDE installations should select the same ESP32 core, library
version, and board target.

Arduino CLI setup:

```sh
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.10
arduino-cli lib install "Simple FOC@2.4.0"
```

Compile the sketches:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 \
  firmware/MKS_Bilateral_Link_Test

arduino-cli compile --fqbn esp32:esp32:esp32 \
  firmware/MKS_Current_Sense_Diagnostic

arduino-cli compile --fqbn esp32:esp32:esp32 \
  firmware/MKS_Parallel_Mirror
```

Upload one sketch at a time:

```sh
arduino-cli upload -p <serial-port> --fqbn esp32:esp32:esp32 \
  firmware/MKS_Parallel_Mirror
```

Disconnect motor power while flashing. Use USB-A-to-USB-C with this board
revision; USB-C-to-USB-C was not reliable on the tested boards.

## 6. Pair-Specific Configuration

The published handoff firmware is calibrated for the two tested nodes. These
values are not universal:

| Setting | Node A | Node B |
| --- | --- | --- |
| Wi-Fi station MAC | `30:C9:22:5F:5D:1C` | `30:C9:22:5F:5D:24` |
| Electrical zero | `0.630083 rad` | `5.940915 rad` |
| Sensor direction | `CCW` | `CCW` |

The firmware identifies A or B from the local MAC address. A replacement ESP32,
motor, encoder, changed phase order, or changed magnet alignment requires:

1. recording the replacement board's station MAC
2. updating `MAC_A` or `MAC_B` in both active sketches
3. building a controlled development firmware with `HANDOFF_MODE=false`
4. running a fresh FOC calibration with the mechanism free to move
5. recording the reported direction and `zeroE` value
6. updating the stored constants only after repeatable calibration results
7. reflashing both nodes with the same protocol version
8. repeating all link, current, motion, timeout, and thermal checks

Calibration causes motor movement. Do not run it in an assembled user-facing
mechanism or with hands inside the motion envelope.

## 7. Staged Bring-Up

### Stage 1 — Visual and electrical inspection

- confirm the board silkscreen says `Makerbase ESP32 FOC Mega`
- inspect battery polarity and connector strain relief
- verify that each node has its own protected `3S` pack
- keep motor power disconnected while checking USB communication
- keep the mechanism mechanically secured and unobstructed

The onboard RGB LED is unusable on both tested boards because its `VDD` and `DI`
nets are swapped at the footprint. An unlit LED is expected and is not a status
signal.

### Stage 2 — Encoder and radio link

Flash `MKS_Bilateral_Link_Test` to both nodes. This sketch keeps the motor
drivers disabled.

Pass criteria:

- both nodes identify as A or B, not unknown hardware
- `AS5048A: OK`
- local angle changes continuously and in the expected direction
- receive count increases on both nodes
- packet age normally remains well below the `250 ms` timeout

### Stage 3 — Current-sense offsets

With the motor driver not switching, run
`MKS_Current_Sense_Diagnostic`.

The tested zero-current reference levels were approximately `1685 mV` on node A
and `1676 mV` on node B. Treat these as historical reference values, not
universal acceptance limits. Investigate unstable, saturated, or markedly
different readings before enabling drive.

### Stage 4 — Main controller

Flash `MKS_Parallel_Mirror` to both nodes. Keep both mechanisms free during the
startup interval.

Expected serial state:

- `radio=OK`
- `storedFOC=OK`
- `currentSense=OK`
- `protocol=7`
- the two nodes report matching profiles
- automatic arm occurs only after peer synchronization and the 5-second delay

### Stage 5 — Fault checks

Before any user-facing test:

- confirm manual stop disables both nodes
- power down one node and confirm the peer disables after link timeout
- confirm handoff mode remains latched off after a runtime link fault
- power-cycle both nodes and confirm synchronized startup
- confirm automatic shutdown after the 10-minute session limit

## 8. Acceptance Boundary

Successful compilation and short motion tests do not make the device ready for
unattended, clinical, or extended operation. The remaining mechanical,
electrical, thermal, and user-test work is tracked in the
[prototype checklist](prototype-checklist.md).

The current validated operating boundary is:

- two specified MKS Mega boards
- two specified GM3506/AS5048A motor assemblies
- separate protected `3S` packs
- protocol version 7 on both nodes
- supervised sessions no longer than 10 minutes
- approximately 180–200 degrees of intended travel from the startup pose

## 9. Historical Firmware

The first-generation `FOC_ESP32_V1.1` sketch is retained under
[`firmware/legacy`](../firmware/legacy/README.md) for research history. Its
pinout, MAC addresses, startup behavior, and safety model do not apply to the
current Makerbase MKS ESP32 FOC Mega system.
