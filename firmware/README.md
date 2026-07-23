# Firmware

The firmware is organized as a set of Arduino sketches for the Makerbase MKS
ESP32 FOC Mega platform. Each sketch has a separate directory so it can be
opened directly in the Arduino IDE.

## Sketches

| Sketch | Purpose | Motor drive |
| --- | --- | --- |
| [`MKS_Parallel_Mirror`](MKS_Parallel_Mirror/MKS_Parallel_Mirror.ino) | Main autonomous bilateral controller and supervised handoff build | Enabled after synchronized startup |
| [`MKS_Bilateral_Link_Test`](MKS_Bilateral_Link_Test/MKS_Bilateral_Link_Test.ino) | Encoder and ESP-NOW link validation without motor actuation | Disabled |
| [`MKS_Current_Sense_Diagnostic`](MKS_Current_Sense_Diagnostic/MKS_Current_Sense_Diagnostic.ino) | Standalone current-sense offset and ADC diagnostic | Disabled |
| [`MKS_RGB_Diagnostic`](MKS_RGB_Diagnostic/MKS_RGB_Diagnostic.ino) | Diagnostic experiments for the onboard addressable LED | No motor control |

## Main Handoff Build

`MKS_Parallel_Mirror` currently uses protocol version 7 with
`HANDOFF_MODE=true`. In this configuration it:

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

- Arduino IDE or Arduino CLI
- Espressif ESP32 Arduino core
- SimpleFOC library
- an ESP32 board target compatible with `ESP32-WROOM-32E`

Open the required `.ino` file from its containing directory and compile it for
the configured ESP32 target. Flash the same main sketch to both nodes; each node
selects its identity from its own MAC address.

## Safe Bring-Up Order

1. Validate encoder readings and peer communication with
   `MKS_Bilateral_Link_Test`.
2. Check current-sense offsets with `MKS_Current_Sense_Diagnostic`.
3. Verify the calibration constants and hardware mappings in the main sketch.
4. Flash `MKS_Parallel_Mirror` to both nodes.
5. Follow the [supervised operation guide](../docs/HANDOFF.md).

Do not use the main controller as a generic motor test. Incorrect phase order,
encoder direction, electrical calibration, or power limits can cause sudden
motion or excessive current.
