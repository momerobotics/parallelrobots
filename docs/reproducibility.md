# Reproduction Guide

This guide describes the exact tested configuration of the Parallel Mirror
Steer-by-Wire prototype. It separates reproduction of the validated two-node
system from adaptation to replacement hardware.

For a completely new pair, follow the current [manual commissioning guide](new-system-commissioning.md)
and fill in an [assembly record](commissioning-record-template.md).
The values below describe the original pair, not the September replacement pair.

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
arduino-cli upload -p <serial-port> --fqbn esp32:esp32:esp32:UploadSpeed=115200 \
  firmware/MKS_Parallel_Mirror
```

Always use **115200 baud for uploads**, including diagnostics and replacement
boards. Higher speeds have repeatedly failed on this project; do not retry the
default 921600 baud before using the proven rate. In Arduino IDE select
Tools > Upload Speed > 115200. Serial-monitor baud is a separate setting.
This standing user instruction was confirmed on 2026-09-14.

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

The firmware identifies A or B from the local station MAC address. New physical
motor/encoder assemblies require individually measured electrical zeros and
directions even when their model numbers match. With only a compatible board
replacement, an unchanged motor/encoder assembly and unchanged phase order can
reuse its accepted motor calibration; the new board still needs identity,
current-sense, VIN and functional checks.

The main sketch currently forces CCW in both stored setup and its blocking
`c` calibration path. Setting `HANDOFF_MODE=false` alone is not a general
new-motor calibration procedure. Follow the calibration gate and manual
configuration instructions in the [new-system guide](new-system-commissioning.md).

The September replacement pair has these recorded station identities:

| Item | A | B |
| --- | --- | --- |
| Station MAC | `30:C9:22:5F:5E:2C` | `88:57:21:19:D5:70` |
| Accepted electrical zero | `1.566572 rad` | `4.843634 rad` |
| Accepted encoder direction | `CCW` | `CCW` |

The main/link source now uses these replacement MACs and the main uses the
accepted zeros above. The original pair's earlier table is retained for history.
See the session record for the stronger-profile commissioning status.

## 7. Staged Bring-Up

Use the [detailed manual procedure](new-system-commissioning.md) for commands,
logs, limits, pass criteria and the configuration checklist:

1. Inspect wiring, polarity, phase continuity and mechanical attachment; label
   each board/motor/encoder and battery separately.
2. Run `MKS_Startup_Diagnostic` to record station MACs and validate stationary,
   bidirectional and cable-motion encoder readings without motor drive.
3. Run `MKS_Current_Sense_Diagnostic` on USB and actual supply; record idle offsets.
4. Use the idle phase diagnostic to compare VIN with a meter, then run the
   v2 six-vector test on one secured motor at a time.
5. Establish repeatable FOC direction/zero and independently verify both directions.
   Standalone calibration v3 and independent FOC verification passed on both
   September assemblies; repeat those checks for another new pair.
6. Update both MAC arrays in main and link sketches and validate the no-drive
   radio diagnostic on both nodes. This can precede step 5; main drive cannot.
7. Integrate accepted calibrations and use a controlled main build with automatic
   startup disabled on both nodes. Verify gentle-profile coupling and stop behavior.
8. Only after these checks, restore the handoff configuration and perform its
   startup, link-loss, session-timeout, thermal and actual-mounting acceptance tests.

The September diagnostics used ESP32 core 3.3.11 with CLI 1.4.1 and Simple FOC
2.4.0. This differs from the original core 3.3.10 handoff environment above;
record and validate the environment used for the eventual new main build.

Detailed results and pending checks are in the
[September startup record](startup-2026-09-14/README.md).

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
