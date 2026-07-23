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

## July 14, 2026 - MKS ESP32 FOC Mega Port

The bilateral controller was ported to and tested on two Makerbase MKS ESP32
FOC Mega boards with iPower GM3506 motors and AS5048A SPI encoders.

Validated configuration:

| Item | Value |
| --- | --- |
| Motor PWM | `GPIO32 / GPIO33 / GPIO25` |
| Motor enable | `GPIO12` |
| AS5048A SPI | `SCK 18 / MISO 19 / MOSI 23 / CS 5` |
| Motor pole pairs | `11` (`24N / 22P`) |
| ESP-NOW update period | `4 ms` |
| Measured packet age | typically `1-4 ms` |
| Control voltage limit | `0.25 V` |

Stored FOC calibration was verified on both nodes. Bidirectional haptic coupling
was tested from each motor, with the final reverse-direction test settling to
about `0.04 rad` of position difference without a protection trip.

The MKS firmware also includes link timeout, speed and following-error limits,
torque slew limiting, automatic disarm, and a `150 ms` zero-position
synchronization window. The synchronization window prevents a stale packet from
causing a false following-error trip when the two nodes are armed sequentially.

The M0 inline current-sense circuit was verified from the official Makerbase
schematic and on the physical boards. It uses `10 mOhm` shunts, `INA240A2`
amplifiers with gain `50`, and ESP32 ADC inputs `GPIO39 / GPIO36`. Measured
zero-current reference levels were about `1685 mV` on node A and `1676 mV` on
node B. During a bilateral load test, the held peak currents were `0.204 A` and
`0.176 A`; steady holding current was approximately `0.05-0.07 A`.

The first current-monitored firmware retained voltage-mode torque control,
monitored both measured phases, reconstructed the third phase, and disarmed
after sustained phase current above `0.75 A`. A standalone non-switching ADC
test is available in `firmware/MKS_Current_Sense_Diagnostic/`.

### Autonomous foldback prototype

The controller was subsequently changed from manually armed, latched motion
protection to autonomous startup and continuous foldback regulation. Both nodes
now arm without a USB host after a `1.5 s` peer synchronization period. Position
error is bounded before entering the spring model, drive torque fades between
`4-7 rad/s`, and phase-current foldback acts between `0.30-0.50 A`. A real radio
timeout still disables the power stage and initiates automatic recovery when the
link returns.

The first loaded test of the `1.2 V` control profile reached held peak currents
of `0.360 A` on node A and `0.441 A` on node B. Current foldback intervened on
both nodes, neither node latched off, and both returned to near-zero position and
torque after release. The configured `7 V` value is the power-stage ceiling, not
an unconditional motor torque command.

### Mega RGB hardware erratum

Macro photographs identified the indicator between the sensor connectors as an
`XL-3528RGBW-WS2812B`. RMT tests covered standard WS2812 timing, the Xinglight
datasheet timing, and alternate free GPIOs on both boards without producing
light. Static probing then confirmed that `GPIO2` reaches the package's
top-left `VDD` lead, while the top-right `DI` lead is permanently at `3.3 V`.
The PCB footprint swaps the LED supply and data nets on both tested boards.
This cannot be corrected in firmware; the upper leads must be lifted and
cross-wired, or an external status LED must be used.

### Input-voltage calibration

The boot-time voltage monitor was calibrated with the motors stopped against
multimeter readings taken directly at each board's green power input. Node A
reported `11.55 V` at an actual `11.25 V`; node B reported `12.71 V` at an
actual `12.42 V`. The resulting individual divider factors (`8.279` and
`8.306`) agree within `0.33%`, so the shared `VIN_SCALE` was changed from
`8.50` to `8.29`. Expected readings after calibration are approximately
`11.27 V` and `12.40 V` for the same input voltages.

### Motion refinement

The first feel-oriented refinement removes the `0.10 V` discontinuity at the
edge of the position deadband. Static-friction compensation now rises smoothly
over `0.060 rad`, reducing the center notch while retaining useful breakaway
torque. Each node also transmits its filtered velocity and applies damping to
the relative velocity between the two motors. Common motion therefore receives
less drag, while rapid opposing motion receives additional damping.

The first dynamic test of this revision reached `17.75 rad/s`. Current foldback
kept the observed peaks to `0.435 A` on node A and `0.497 A` on node B without
latched shutdown, and the mechanism settled with about `0.04 rad` position
difference. At that small residual error the position integrator could still
build repeating correction pulses. Integration is therefore enabled only above
`0.10 rad`; below that threshold its contribution decays at `0.80 V/s`. The
foldback counter now counts entries into limiting rather than control-loop
iterations spent limiting.

A subsequent one-sided load test held node A while node B moved. Neither node
latched off; measured peaks were `0.391 A` and `0.427 A`. The near-center torque
remained steady after release, confirming that the integrator pulse was removed.
The test also exposed rapid soft-limit crossings caused by using raw instantaneous
phase-current samples. Foldback now uses a peak envelope with immediate attack,
an `80 ms` release time constant, and `0.95 / 0.99` entry/exit hysteresis. A raw
sample at or above the `0.50 A` hard limit still commands zero drive immediately.

The repeated one-sided hold test validated the envelope: each node recorded one
foldback entry instead of hundreds of threshold crossings. Peak phase currents
were `0.391 A` on node A and `0.425 A` on node B, with no hard-limit event,
latched shutdown, or communication disturbance. This foldback behavior is the
new baseline for subsequent feel and force-profile tuning.

### Synchronized force profiles

The controller now provides `gentle`, `normal`, and `strong` profiles. Normal is
the autonomous startup default and retains the validated `1.20 V` behavior.
Gentle reduces command voltage, virtual spring strength, friction compensation,
and integral action. Strong raises the command ceiling to `1.60 V` and increases
spring and damping gains, while retaining the same measured `0.30-0.50 A`
current envelope. Serial commands `g`, `n`, and `s` select profiles during
development. The selected profile is included in every ESP-NOW packet; torque
is held at zero whenever the two nodes disagree, preventing asymmetric gains.

The first direct normal-to-strong comparison completed without a hard-current
event, latched shutdown, or communication fault. Normal reached `0.408 A` on
node A and `0.412 A` on node B, with a final relative position error of about
`0.073 rad`. Strong reached `0.453 A` and `0.425 A`; it added one foldback entry
on A and eleven on the actively loaded B, and settled at about `0.057 rad`
relative error. The hand-driven trajectories were not identical, so this is a
safety and behavior validation rather than a calibrated force comparison.
Runtime profile selection now resets peak-current, foldback-event, and dynamic
scaling telemetry so later profile tests start from clean counters.

### Structured strong-profile motion test

A roughly three-minute structured test covered slow one-sided following in both
directions, release and settling, resisted motion, common-mode motion, and rapid
short reversals. Node A reached `0.439 A` with 15 foldback entries; node B
reached `0.453 A` with 24 entries. Neither node reached the `0.50 A` hard-current
threshold, disarmed unexpectedly, or lost its peer. Packet age remained mostly
within `0-4 ms`, with an observed isolated `6 ms` sample. During the rapid
reversal section node B reached approximately `8.35 rad/s`; the speed scale fell
to zero as designed, so the controller stopped adding drive above its configured
speed envelope. After release, the two reported positions differed by about
`0.067 rad` and the remaining correction command was approximately `0.06 V`.
The test validates the strong profile for continued prototype tuning, while the
foldback counts show that repeated fast reversals are already using the intended
soft current boundary.

Subjective evaluation found no vibration, but detected a slightly stepped,
deliberate return when one motor was held, the other displaced, and then
released. To make the elastic link more immediate without raising its current
envelope, the position deadband was reduced from `0.035` to `0.025 rad` and the
normal torque slew rate increased from `3` to `6 V/s`. Strong-profile spring
gain increased from `1.55` to `2.00`, friction compensation from `0.13` to
`0.15 V`, and integral gain from `0.20` to `0.24`. Local viscous damping was
reduced to `0.020`, while relative coupling damping increased to `0.040`, aiming
for less common-motion drag and a tighter connection between opposing motions.
The `0.30-0.50 A` current foldback envelope remains unchanged.

Free-motion evaluation described the revised response as substantially livelier
and more natural, with some remaining stepped feel and a still-deliberate return
from large angular offsets. The test reached short raw-current peaks of `0.540 A`
and `0.524 A`, with 39 and 37 foldback entries but no unexpected disarm or link
fault. Since measured packet age remained around `0-4 ms`, communication was not
identified as the primary source of the stepped feel; motor cogging, voltage-mode
torque control, and repeated foldback remain likely contributors. As a final
low-risk refinement, the ESP-NOW period was reduced from `4` to `2 ms`, the
velocity filter time constant from `30` to `20 ms`, and the spring exponent from
`1.35` to `1.20`. This increases update continuity and medium-to-large-error
spring force without raising the `1.60 V` strong-profile command ceiling or the
configured current envelope.

The next evaluation found little force difference between approximately `20`
and `200 degrees`. Inspection showed that control error was intentionally
clamped at `0.9 rad` (`52 degrees`), and the strong voltage curve saturated near
the same point. The strong profile now uses a progressive virtual spring: its
ordinary spring section extends to `0.9 rad`, followed by a quadratic end-stop
term that reaches full force at `3.5 rad` (about `200 degrees`). The strong
ceiling is `2.20 V`; its reduced base gain preserves low-angle compliance while
the end-stop becomes increasingly difficult to wind farther. The GM3506 vendor
lists a `1 A` load-current point at `12 V`; the prototype remains below this,
with soft/hard phase-current thresholds cautiously raised to `0.40 / 0.65 A`.
This higher-force mode still requires short tests and manual temperature checks
because the boards do not measure motor winding temperature.

The first progressive-end-stop test remained below the new hard threshold:
node A peaked at `0.461 A` and node B at `0.546 A`. The soft foldback envelope
recorded 150 and 64 entries during repeated winding and return, with no link
fault or unexpected disarm. The final reported position difference was about
`0.006 rad` (`0.34 degrees`). This is the final validated firmware state for the
day; longer-duration thermal validation is still required before treating the
higher-force strong profile as a continuous-duty operating mode.

Subjectively, the progressive curve produced the intended convincing spring
feel up to roughly `200 degrees`. Beyond that point and over multiple turns the
return became slower and felt fatigued. This matches the present design: the
virtual end-stop reaches full command and control error is capped at `3.5 rad`,
while sustained multi-turn motion invokes speed and current foldback. Multi-turn
operation is outside the expected working range, but remains a tracked edge
case. Future work should define an explicit travel policy and add thermal or
energy-budget derating rather than merely increasing current further.

No perceptible motor heating, vibration, or abnormal motion noise was observed
during the short progressive-spring and multi-turn edge-case tests. This is a
positive qualitative result, but does not replace the planned instrumented
continuous-duty thermal test.

### Software thermal-load estimate

The controller now adds a non-latching long-duration protection layer based on
an exponentially filtered phase-current-squared estimate. Its `45 s` time
constant preserves short force peaks while approximating sustained winding
load. The resulting torque scale fades between `0.45 A` and `0.60 A` estimated
thermal current and automatically recovers as the estimate cools. Telemetry
reports `thermalA` and `thermalScale`. This is deliberately conservative and is
not a substitute for a physical winding or driver temperature sensor.

A six-minute mixed endurance test exercised continuous `30-90 degree` motion,
alternating held offsets, and brief `150-200 degree` excursions. Node A reached
`0.628 A` and 148 fast-foldback entries; node B reached `0.619 A` and 240
entries, both remaining below the `0.65 A` hard threshold. At shutdown the
thermal-current estimates were approximately `0.126 A` and `0.124 A`, with
`thermalScale=1.00` on both nodes. Neither motor nor power board felt perceptibly
warm, and no abnormal noise, vibration, link fault, or unexpected disarm was
observed. This validates the estimator under representative short-duration use;
the checklist's instrumented `10-15 minute` continuous-duty test remains open.

### Persistent operating profile

The selected gentle, normal, or strong profile is now stored in ESP32 NVS and
restored before radio startup. A standalone power cycle therefore returns both
nodes to their last selected behavior without a USB host. Flash is written only
when the profile actually changes. Invalid stored values fall back to normal,
and the existing peer-profile agreement check continues to suppress torque if
the two boards restore different profiles.

After charging, a fresh boot measured `12.47 V` on node A against `12.48 V` at
the input with a multimeter; node B measured `12.41 V`. The resulting startup
estimates of `94%` and `91%` agreed with the charger's roughly `90%` indication.
Earlier lower percentages were stale pre-charge boot readings, confirming that
the voltage calibration is sound and that the documented boot-only limitation,
not scale error, caused the apparent discrepancy.

### Startup and peer compatibility hardening

Automatic startup is now inhibited below the existing conservative `11.1 V`
minimum rather than only below the absolute `10.5 V` empty-pack endpoint. Every
ESP-NOW packet also carries an explicit protocol version. Packets from an
incompatible firmware version are discarded before updating peer state, so a
partial firmware update cannot produce torque between mismatched controllers.
Telemetry reports both local and peer protocol versions.

The compatibility guard was validated with a staged update. After updating only
node A, it remained `armed=0` with `ageMs=never`, `rx=0`, and `peerProtocol=0`
while node B still ran the prior packet format. Once B was updated, both nodes
reported `protocol=1`, `peerProtocol=1`, restored their persisted strong profile,
and resumed normal `0-2 ms` peer communication.

### Coordinated run intent

Protocol version `2` adds an explicit run-enabled flag. A manual stop issued to
either node propagates to its peer and disables both power stages. Starting one
node gives it a short local synchronization window; the peer follows only the
stopped-to-running edge, avoiding a start request that could echo indefinitely.
A valid edge clears the coordinated stop latch and permits peer-synchronized
automatic arming. A node below the boot-voltage threshold cannot be restarted by
its peer and propagates its stopped run intent to the healthy node.

The coordinated state machine was validated on both powered motor controllers.
With both nodes armed, sending `x` only to A produced `manual stop` on A and
`peer stop` on B, with both reporting `armed=0` and `auto=0`. Sending `a` only
to B then restored both nodes to `armed=1`, `auto=1`, and cleared A's manual
stop latch. Finally, sending `x` only to B propagated `peer stop` to A. Both
drivers were left disabled after the test; peer packet age remained typically
`1-3 ms` throughout.

### Rejected directional travel-policy experiment

An experimental strong-profile policy added outward velocity damping from
`2.8 rad` to `3.5 rad` and retained a `30%` return-drive floor near the boundary.
It did not produce a meaningful subjective improvement over the established
progressive spring, but introduced a position-periodic clicking sensation. The
experiment was therefore removed and the previous validated spring controller
restored without increasing voltage or current limits.

The first bidirectional `90/160/180-200 degree` hand test completed without an
unexpected disarm. Node A reached `0.481 A` and 194 current-foldback entries;
node B reached `0.631 A` and 75 entries, remaining just below the `0.65 A` hard
threshold. Post-test thermal estimates were approximately `0.18 A` and `0.16 A`
with `thermalScale=1.00`, so no long-duration derating was active. Both nodes
were stopped through the coordinated stop command after collecting telemetry.
The result is retained here as a rejected experiment rather than a validated
control change.

After restoring the prior controller on both nodes, the same slow bidirectional
`0-200 degree` sweep no longer produced the periodic clicking sensation. Peak
currents also fell to `0.437 A` on A and `0.511 A` on B, compared with `0.481 A`
and `0.631 A` during the rejected experiment. The rollback is therefore the
current validated firmware state. Both nodes were left in coordinated stop.

### Bidirectional reset recovery

One-sided controller resets were validated in both directions while the peer
remained powered and armed. Resetting A caused B to disable on `link timeout`;
resetting B likewise caused A to disable, with the first sampled stopped status
showing `ageMs=264` against the `250 ms` timeout. During each peer reboot the
healthy node remained disabled. After compatible packets resumed, both nodes
re-zeroed, armed locally, waited for peer arm, and returned to synchronized
operation without manual intervention. A coordinated stop was issued after the
test. This establishes the fault-recovery baseline needed before raising the
strong-profile output.

### Experimental boost profile

Protocol version `3` adds a manually selected `boost` profile while retaining
the validated `strong` profile unchanged. Boost uses the same progressive spring
shape with a `2.50 V` command ceiling, slightly higher base spring gain, and a
profile-specific `0.45-0.70 A` fast current-foldback envelope. The existing
software thermal estimator and `0.45-0.60 A` sustained-load derating remain
unchanged and retain final authority. Boost is selected with `b` on both nodes
and deliberately is not persisted to NVS, so the next standalone boot returns
to the last validated gentle, normal, or strong profile.

The first `30-45 s` boost hand test completed without an unexpected disarm.
Node A peaked at `0.418 A` with no foldback entry; node B peaked at `0.555 A`
with 24 foldback entries, remaining well below the boost `0.70 A` hard limit.
Thermal estimates were approximately `0.15 A` on both nodes with
`thermalScale=1.00`. The test ended with a coordinated stop. Subjective force,
smoothness, noise, and temperature feedback are still required before boost is
promoted beyond experimental status.

The first subjective boost tests reported a repeatable position-periodic click,
approximately seven events over `180 degrees`, while the lower strong profile
remained smooth. The GM3506 encoder variant is consistently specified as
`24N/22P`, so the configured 11 pole pairs remain correct. A follow-up boost
experiment changes FOC modulation from space-vector PWM to sine PWM to target
low-speed commutation torque ripple without raising voltage or current limits.

The sine-PWM comparison reduced the subjective ripple from roughly seven to
five or six weaker clicks over `180 degrees`. Peak currents remained moderate at
`0.514 A` and `0.589 A`. Inspection of SimpleFOC 2.4.0 then showed that the
existing `c` command reused the configured electrical zero instead of measuring
it again. The command now explicitly clears the offset, retains the validated
CCW sensor direction, and performs a fresh offset alignment at `1.5 V` before
printing the measured value. This enables a controlled per-motor electrical-zero
recalibration before further boost tuning.

Two forced measurements per motor were repeatable: A returned `0.634301` and
`0.625865 rad`; B returned `5.947243` and `5.934587 rad`. Their mechanical
spread was only about `0.04-0.07 degrees`. Firmware protocol version `4` stores
the pairwise means, `0.630083 rad` for A and `5.940915 rad` for B. Compared with
the previous stored values, the corrections are about `1.38 degrees` and
`1.65 degrees` mechanical respectively, large enough to plausibly explain why
commutation ripple became apparent only in the higher-output boost profile.

The protocol-4 boost retest with the averaged electrical-zero values produced
a noticeably faster return and made the remaining position-periodic clicking
less intrusive, although it did not remove it completely. No meaningful motor
or controller heating, vibration, or abnormal noise was observed. Post-test
telemetry showed peak currents of `0.558 A` on A and `0.574 A` on B, with only
33 and 22 current-foldback entries. Thermal estimates had already decayed to
approximately `0.08 A` and `0.06 A`; both nodes retained
`thermalScale=1.00`. This confirms that the revised offsets are an improvement
and that the current thermal policy still has comfortable margin. Both nodes
were left in coordinated manual stop. The residual click should next be
investigated as low-speed commutation/control ripple rather than addressed by
raising the current ceiling alone.

With both drivers disabled, neither motor exhibited the clicking during a
manual rotation. The effect is therefore generated by active commutation, not
by passive magnetic cogging or the mechanism. Protocol version `5` begins an
isolated PWM-frequency comparison at the ESP32 MCPWM backend's supported
maximum of `40 kHz`, up from `25 kHz`. Sine PWM, calibrated electrical offsets,
control gains, voltage ceilings, and current/thermal limits remain unchanged.

The `40 kHz` comparison reduced the subjective click count from five or six to
roughly two over `180 degrees`. The remaining events changed position and count
with hand speed, ruling out a fixed rotor-angle defect. Telemetry recorded 12
soft current-foldback entries on A and 34 on B, despite modest peaks of
`0.541 A` and `0.580 A` and thermal estimates near `0.11 A`. Protocol version
`6` therefore raises only the experimental boost soft-current threshold from
`0.45 A` to `0.52 A`. The `0.70 A` hard ceiling and all sustained thermal
limits remain unchanged, allowing a direct test of whether repeated soft
foldback was producing the residual tactile steps.

The protocol-6 hand test felt more continuous overall, with the clearest
improvement at higher hand speed. Repeated subjective comparisons had reached
the point of tester fatigue, so no finer ranking is claimed. Peak currents were
`0.663 A` on A and `0.653 A` on B, below the unchanged `0.70 A` hard limit;
thermal estimates were only about `0.07-0.08 A` with no thermal derating. Soft
foldback entries fell to 13 and 21. Both nodes were stopped. Further tuning will
use a repeatable logged A/B procedure instead of relying on many consecutive
free-form hand comparisons.

### Supervised handoff build

Protocol version `7` packages the latest validated tactile configuration for a
same-day supervised handoff. Handoff mode ignores profile values left in NVS,
locks both nodes to boost, increases the automatic startup delay to 5 seconds,
disables calibration/manual-zero/profile commands, and enforces a 10-minute
session limit. A runtime ESP-NOW timeout or peer stop now latches both nodes off;
normal operation resumes only after both controllers are fully power-cycled (or
an explicit development serial start). A forced one-node reset confirmed that
the healthy peer entered `stopLatch=1` on link timeout and did not re-arm when
communication returned. Initial protocol-7 startup was also verified with both
nodes armed, matched, and idle at `11.69 V` and `11.68 V` respectively.
