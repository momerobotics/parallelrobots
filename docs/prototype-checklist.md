# Testable Prototype Checklist

This checklist separates the validated control proof of concept from the work
still required for a repeatable, mechanically integrated prototype.

## Validated

- AS5048A SPI position sensing on both MKS ESP32 FOC Mega boards
- stored FOC calibration for both GM3506 motors (`11` pole pairs)
- bidirectional ESP-NOW haptic coupling and automatic peer synchronization
- autonomous startup without a USB host
- phase-current measurement with profile-specific foldback up to a `0.70 A`
  hard ceiling in the supervised handoff profile
- speed and position foldback without latched motion-error stops
- synchronized gentle, normal, and strong software profiles
- safe power-stage shutdown and recovery after a radio timeout
- startup VIN measurement on `GPIO13`
- packet protocol versioning with incompatible-peer rejection
- bidirectional one-node reset, link-timeout shutdown, and automatic recovery
- protocol-7 supervised handoff mode with fixed profile, 5-second startup,
  10-minute session limit, locked tuning commands, and latched runtime link faults
- versioned build instructions and a staged reproduction procedure

## P0 - Before Unattended or Extended Testing

- rework the swapped onboard RGB `VDD/DI` nets or add an external status LED
- calibrate VIN against a multimeter at full, nominal, and low 3S battery voltage
- use a protected 3S pack or BMS with cell-level undervoltage protection
- add an accessible hardware power switch or emergency stop and an appropriate fuse
- add mechanical travel limits and prevent fingers, cables, and linkages entering pinch points
- perform a stationary `10-15 minute` thermal test while logging motor and driver temperature
- verify current readings against an external meter under representative load

## P1 - Before Repeatable User Tests

- provide continuous battery monitoring using an ADC1 pin or an external ADC/fuel gauge
- add motor and power-stage temperature sensing with torque derating
- test startup with one node missing, delayed, or powered from a different battery
- test ESP-NOW dropout, range, interference, and automatic recovery
- add a physical selector and external status indication for the validated software profiles
- store and validate calibration version data
- run repeated power-cycle, stall, rapid-motion, and one-sided-load tests
- document the mechanical zero pose, allowed range, payload, and maximum user force
- define multi-turn behavior: travel limit, accumulated spring energy, and thermal derating

## P2 - Prototype Packaging

- strain relief and keyed connectors for motor, encoder, battery, and programming cables
- guarded electronics and battery enclosure with ventilation
- accessible charge connector and battery state indication when motors are off
- mechanical assembly drawings, dimensions, fasteners, and acceptance tolerances

## Battery Measurement Limitation

The Mega board's VIN divider is connected to `GPIO13`, an ESP32 ADC2 channel.
ADC2 is shared with Wi-Fi, so the current firmware measures battery voltage only
before ESP-NOW starts. The serially reported percentage is therefore a startup
estimate, not a live fuel gauge. A protected pack remains mandatory; continuous
software cutoff requires moving the divider to ADC1 or adding an external
voltage monitor.
