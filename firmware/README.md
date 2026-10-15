# Firmware

The firmware is organized as a set of Arduino sketches for the Makerbase MKS
ESP32 FOC Mega platform. Each sketch has a separate directory so it can be
opened directly in the Arduino IDE.

Motor commutation and torque-voltage control use
[Arduino SimpleFOC](https://github.com/simplefoc/Arduino-FOC). The bilateral
virtual-spring concept originated in SimpleFOC's
[Haptics — Steer by Wire example](https://docs.simplefoc.com/haptics_examples);
this firmware extends it with two wireless controller nodes, synchronized state,
current/speed/thermal foldback, and supervised fault handling.

## Sketches

| Sketch | Purpose | Motor drive |
| --- | --- | --- |
| [`MKS_Parallel_Mirror`](MKS_Parallel_Mirror/MKS_Parallel_Mirror.ino) | Main bilateral controller, current supervised commissioning build | Explicit coordinated start only |
| [`MKS_Bilateral_Link_Test`](MKS_Bilateral_Link_Test/MKS_Bilateral_Link_Test.ino) | Encoder and ESP-NOW link validation without motor actuation | Disabled |
| [`MKS_Current_Sense_Diagnostic`](MKS_Current_Sense_Diagnostic/MKS_Current_Sense_Diagnostic.ino) | Standalone current-sense offset and ADC diagnostic | Disabled |
| [`MKS_Startup_Diagnostic`](MKS_Startup_Diagnostic/README.md) | Station MAC and independent encoder/cable checks | Disabled |
| [`MKS_Phase_Startup_Test`](MKS_Phase_Startup_Test/README.md) | v2 short six-vector startup test, passed on the September pair | Explicit `w` only |
| [`MKS_FOC_Startup_Calibration`](MKS_FOC_Startup_Calibration/README.md) | v3 repeat calibration and independent FOC verification passed on the September pair | Explicit `c` / `v` only |

The earlier `FOC_ESP32_V1.1` prototype is preserved under
[`legacy/`](legacy/README.md). It is not compatible with the current MKS Mega
pinout.

## Current replacement-pair build

The source currently targets the September pair with accepted individual FOC
zeros (A 1.566572 rad, B 4.843634 rad; both CCW). It is a controlled commissioning
build: `HANDOFF_MODE=false`, initial `autoStartEnabled=false`. Explicit arming is
required. Force acceptance and final autonomous handoff checks remain in progress.
See the [end-of-day baseline](../docs/startup-2026-09-14/end-of-day.md) for the
saved knee10-no-filter configuration and resumption procedure. The
[session record](../docs/startup-2026-09-14/README.md) preserves earlier trials.

The trial boost uses springK=8.00, a 3.50 V command ceiling, 0.008 rad deadband
and 1.10/1.35 A current thresholds. Thermal limits remain 0.45/0.60 A.
These new limits are not yet a validated continuous-use rating. A runtime motor
limit bug that retained the 1 V initialization ceiling has been corrected.

## Main Handoff Build

The intended final handoff configuration uses protocol version 7 with
`HANDOFF_MODE=true` and automatic start enabled. Both settings must be reviewed
and validated together; changing `HANDOFF_MODE` alone does not enable automatic
start in the current source. The intended configuration:

- identifies node A or B from the built-in MAC addresses
- uses the validated AS5048A encoder and M0 motor mappings
- locks both nodes to the tested boost profile
- waits 5 seconds before synchronized automatic arming
- limits a session to 10 minutes
- disables calibration, manual-zero, and profile-selection commands
- latches both nodes off after a runtime communication fault

The main sketch is hardware-specific. Review its pin definitions, MAC addresses,
calibration constants, and power limits before using it on different hardware.

## Build Requirements

- Arduino IDE or Arduino CLI `1.4.1`
- Espressif ESP32 Arduino core: original `3.3.10`; October compile check `3.3.12`
- Simple FOC `2.4.0`
- `ESP32 Dev Module` target (`esp32:esp32:esp32`)

Open the required `.ino` file from its containing directory and compile it for
the configured ESP32 target. Flash the same main sketch to both nodes; each node
selects its identity from its own MAC address.

Always upload at **115200 baud**. With Arduino CLI use
`--fqbn esp32:esp32:esp32:UploadSpeed=115200`; in Arduino IDE choose Upload
Speed 115200. Higher speeds have repeatedly failed. Do not try them first,
including for new boards or diagnostic sketches.

The original main and two original diagnostics compiled with this version set.
The September startup diagnostics used ESP32 core 3.3.11; record the actual
environment for each build. All six current sketches also compiled on 2026-10-15
with core 3.3.12 and Simple FOC 2.4.0; this was a compile-only check, with no
firmware uploads or hardware trials. The archived binaries remain unchanged. See the
[reproduction guide](../docs/reproducibility.md) for installation commands,
signal mapping, pair-specific calibration, and acceptance checks.

## Safe Bring-Up Order

Follow the [manual new-system guide](../docs/new-system-commissioning.md)
and its assembly record:

1. Inspect and label the hardware; capture station MACs and check encoders with
   `MKS_Startup_Diagnostic`.
2. Check idle current offsets and verify VIN with a meter.
3. Run `MKS_Phase_Startup_Test` v2 separately on each motor.
4. Obtain accepted, repeatable FOC calibration and independently verify it.
   Calibration v3 and independent FOC checks passed on the September pair;
   repeat the measurements for each new assembly.
5. Update both station MAC arrays and run `MKS_Bilateral_Link_Test` on both nodes.
   This no-drive stage may also precede FOC calibration.
6. Configure the main with accepted zeros/directions and initially disable
   automatic startup on both boards; validate controlled coupling and stop.
7. Restore and test the handoff configuration using the
   [supervised operation guide](../docs/HANDOFF.md).

The checked-in main/link now target the September pair. Main calibration is held
in source constants, not NVS, and both setup and its `c` path assume CCW.
Do not flash it unchanged on a new assembly. Automatic commissioning and
persistent calibration are [deferred](../docs/future-development.md).

Do not use the main controller as a generic motor test. Incorrect phase order,
encoder direction, electrical calibration, or power limits can cause sudden
motion or excessive current.

## Onboard LED Erratum

Both tested MKS Mega boards have the addressable LED footprint's `VDD` and `DI`
nets swapped. The LED could not be controlled and is not used by the current
firmware. An unlit LED is expected; use serial telemetry for status and fault
diagnosis.


### Supervised legacy comparison profile

The current controlled build also provides `l=legacy`: the original 2 V/rad linear spring with a 0.01 rad deadband and a 3.5 V ceiling. It retains existing current, thermal and communication protections and adds a 30-second trial timeout. This profile is not persisted or selected automatically at startup. See [comparison details](../docs/startup-2026-09-14/legacy-comparison.md) for differences from the supplied historical sketch and trial status.


### Current supervised force profile — knee10-no-filter

`f=force` selects the saved SinePWM bench profile while disarmed. Select it on
both nodes after reboot; force is not persisted in NVS. `a` requests coordinated
start and `x` stops. The session times out after 120 seconds.

- Continuous, symmetric voltage-demand curve: zero through the 0.01 rad
  deadband, 68% at 10 degrees, then 100% at 60 degrees.
- Maximum requested Uq: 5.5 V, also limited to half the boot-estimated supply.
- Current foldback thresholds: 1.75 A soft / 2.00 A hard.
- Thermal-estimate thresholds: 0.90 / 1.10 A with a 45-second EWMA.
- Magnitude attack: 80 V/s; release and reductions are immediate, and reversal
  starts from zero. No angular-error filter or added viscous damping.
- ESP-NOW: up to 100 Hz, one packet in flight; both peers receive 500 ms startup
  grace, with 250 ms suppression of immediate peer-triggered restart after stop.

The final supervised two-minute trial completed and the SinePWM baseline was
restored after an SVPWM comparison. Oscillation remains unresolved. These are
software limits for an experimental bench profile, not measured torque or a
validated continuous or wearable rating. The thermal estimate is not a
physical temperature measurement.

Use the [end-of-day baseline](../docs/startup-2026-09-14/end-of-day.md) for exact
source/binary hashes, external temperature observations and restart instructions.
Earlier linear curves, timeouts, thresholds, and the removed error-filter trial
are retained in the [force trial history](../docs/startup-2026-09-14/force-trial.md)
and source snapshots. They do not describe the current firmware.
