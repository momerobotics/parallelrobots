# Replacement pair startup — 2026-09-14

This is a chronological record. For current installed settings and session closure,
see [End-of-day baseline](end-of-day.md) and [Force trials](force-trial.md).

User confirmed these two USB ports are the replacement controllers and both
batteries were disconnected before flashing. No motor actuation performed.

| Port at test time | Station MAC | Startup result |
| --- | --- | --- |
| `/dev/cu.usbserial-1410` | `30:C9:22:5F:5E:2C` | 5,055 samples; zero invalid frames or unhealthy samples |
| `/dev/cu.usbserial-1430` | `88:57:21:19:D5:70` | 2,129 samples; zero invalid frames or unhealthy samples |

Both received `MKS_Startup_Diagnostic` v1; esptool verified written data hashes.
The first upload on 1410 failed at 921600 baud before image writing. Retry at
115200 succeeded, as did the second board's upload at 115200.

Standing user instruction: always upload at 115200 baud. The user confirms
that higher-speed failures are recurrent, not isolated to today's attempt.
Do not repeat higher-speed upload attempts during routine work.

Environment: Arduino CLI 1.4.1, ESP32 core 3.3.11, esptool 5.3.1.
The previous pair used core 3.3.10; this is a recorded environment change.

Stationary encoder readings were about 177.54 and 295.38 degrees respectively.
Both reported OCF=1, COF=0, compHigh=0, compLow=0, and no parity/device errors.
Attached baseline logs contain the captured serial excerpts.

Pending: physical identity labels A/B, slow manual rotation in both directions,
cable intermittency checks, zero-current offsets, phase/motor tests, fresh FOC
calibration, radio pairing, and bilateral/force validation. Current evidence
only establishes USB flashing and stationary encoder readout; motor operation
and full-motion encoder quality have not yet been tested. Old pair MAC and
calibration constants remain unchanged in the main controller.

The original serial monitors (52269/61454) were lost during the conversation
interruption. The user subsequently performed the requested manual rotation,
but its angle trace was not captured. Do not count that trace as verified.

Monitors were reopened with `dtr=off,rts=off`, recording directly to
`rotation-1410.log` and `rotation-1430.log`. A sandbox restriction on the
monitor's local TCP listener required approved escalation. Active replacement
sessions: 4228 (1410), 25190 (1430).

On reconnect, board uptime and accumulated sample counters were preserved:
1410 reported 62,559 samples at 652,321 ms; 1430 reported 59,681 samples at
622,321 ms. Both still had zero bad frames, parity errors, device error flags,
and unhealthy samples. This supports error-free sampling over the interval,
but does not recover the missing motion trajectory. A short repeated manual
rotation with file logging active is pending.

## Repeated manual rotation — passed basic encoder check

The user repeated manual rotation on both motors while file logging was active.
Both traces show substantial movement in both directions and multiple wraps of
the 0–360 degree angle. All recorded diagnostic samples remained healthy.

| Port | Signed travel range in recorded trace | Samples at review | Error counters |
| --- | --- | --- | --- |
| 1410 | -494.604 to +809.033 degrees | 68,463 | All zero |
| 1430 | -1155.586 to +97.361 degrees | 65,537 | All zero |

Counters: badFrames, parityErrors, errorFlags, unhealthySamples. Both drivers
reported DISABLED throughout. This passes the basic manual bidirectional encoder
check; it does not establish absolute angular accuracy against a reference or
encoder reliability during motor switching. A separate cable-flex test has not
been confirmed. The serial trace reports at 2 Hz, while the internal sampling
and cumulative error counters operate at approximately 100 Hz.

The first serial fragment in each reopened log is incomplete and joined to a
later record; exclude that fragment from time-series analysis. Subsequent full
records capture the repeated motion. Next step: current-sense zero offsets.

## Current-sense diagnostic — USB-only baseline

Both boards now run `MKS_Current_Sense_Diagnostic`, uploaded and hash-verified
at 115200 baud with batteries still disconnected per the last user instruction.
The sketch holds enable and all three phase-control pins LOW and contains no
motor-start command. Station MAC reporting was corrected to use `esp_read_mac`
instead of reversed eFuse-byte printing. Compilation succeeded on core 3.3.11.

| Port | Channel A average | Channel B average | Averaged output ranges |
| --- | --- | --- | --- |
| 1410 | 1660.901 mV | 1661.348 mV | A 1659.32–1662.77; B 1660.03–1663.70 mV |
| 1430 | 1660.310 mV | 1660.767 mV | A 1659.12–1661.80; B 1659.27–1662.41 mV |

USB baseline includes 136 records on 1410 and 41 on 1430. Signals are away
from ADC rails and show modest variation. Offset-derived displayed current
stayed within 0.009 A of startup calibration. This only checks zero-signal
plausibility/stability, not current gain accuracy or sensing under PWM.
See `current-usb-summary.json` for ranges and log byte counts at this boundary.

Current logs: `current-1410.log`, `current-1430.log`. Active serial sessions:
88957 (1410), 46465 (1430), configured 115200 baud and DTR/RTS off. These log
files continue growing. Rotation monitor sessions have been closed; encoder
logs and `rotation-summary.json` are saved. Final rotation counters were 72,975
and 70,097 samples, still without any encoder errors.

Next physical step: reconnect each board's own battery while this disabled
diagnostic remains installed, then review the powered zero-current baseline.
Do not run a motor phase test or FOC calibration until that check is complete
and the mechanisms are secured and free to move.

## Follow-up read — battery connection not yet explicitly confirmed

After the user's short acknowledgement, both logs were live (about 0.2 seconds
old). The latest 80 records averaged 1665.747/1666.383 mV on 1410 and
1666.776/1667.313 mV on 1430. The outputs remain stable and away from ADC rails.
They shifted by about 5–7 mV relative to the USB baseline. Displayed current
therefore has a small zero-offset bias (up to about 0.020 A relative to the
USB-startup calibration), not evidence of motor phase current while disabled.
Fresh zero-current calibration under the actual motor-test supply is needed.
This sketch does not report VIN, so battery presence cannot be inferred from
these readings alone. A clarification about both batteries is pending.

Preliminary review of the historical open-loop sketch found a stale assumption
of 7 pole pairs. The documented GM3506 variant here has 11 pole pairs; do not
upload that historical open-loop test unchanged. No motor-test firmware has
been uploaded and no motor command has been sent.

## Battery confirmation and next test preparation

User explicitly confirmed the battery connection. Powered idle current outputs
remain stable near 1.667 V. `current-battery-summary.json` records a fresh
100-record window per board. The small bias versus USB-startup offsets will be
removed by recalibration while disabled under the actual test supply.

Prepared `firmware/MKS_Phase_Startup_Test`: six 1.0 V phase vectors, 350 ms on /
250 ms off, manual start only, overall 5 s timeout, fresh current zero before
each run, and latched current/encoder/supply/travel faults. Host control-flow
tests pass, as does compilation with core 3.3.11 and Simple FOC 2.4.0.
Build artifacts: `/private/tmp/parallel-phase-startup-build`.

NOT uploaded yet. Both boards still run the disabled current-sense diagnostic.
Next: user disconnects batteries for flashing and secures the motor housings with
rotors free. Upload at 115200; then reconnect batteries, review telemetry and
perform the phase test on one identified board at a time. No movement has been
commanded in this startup session so far.

## Phase firmware uploaded; waiting for battery reconnection

User confirmed batteries disconnected and motor housings secured. Both boards
received the prepared `MKS_Phase_Startup_Test` at 115200 baud, with successful
flash hash verification. Upload transcripts are saved as `phase-upload-1410.log`
and `phase-upload-1430.log`. No firmware changes were made during this upload.

Both running programs report `run=0 drive=0 encoder=OK fault=0 lastStop=boot`.
VIN reads about 1.06 V with external motor power disconnected; this is a
low-end ADC/divider reading, not a battery voltage. Start is inhibited below
11.1 V. Encoder positions at this checkpoint were about 325.7 degrees (1410)
and 8.3 degrees (1430).

Current-sense monitor sessions 88957/46465 are closed. New phase monitors are
70084 (1410) and 77592 (1430), with DTR/RTS off and direct file logging to
`phase-1410.log` and `phase-1430.log`. Do not reopen with truncating redirection
or toggle reset lines unnecessarily. Verify monitor/log freshness before tests.

Next user action: restore each board's own battery, leaving USB connected and
rotors clear. After confirmation, verify fresh VIN/encoder/fault telemetry and
send one `w` test to one board only. Review the complete STOP record and user's
observation before advancing to the other board. A `w` command remeasures the
current zero with outputs disabled immediately before its bounded phase cycle.
No `w` or other motor-start command has been sent yet.

## First motor pulse — stopped by current guard

After the user confirmed battery reconnection, fresh telemetry showed 12.34–12.35 V
on 1410 and 11.14–11.15 V on 1430. Both had healthy encoders and no prior fault.
Port 1430 was left unactuated because its supply was very close to the 11.1 V
stop threshold. Charge/replace that battery before testing that motor; divider
scale on replacement boards has not yet been checked against a meter.

Sent exactly one `w` command through monitor 70084 to port 1410. The driver
recalibrated zero current to 1666.75 / 1667.66 mV (sample spans 16 / 14 mV), then
started run 1 at 12.34 V and 1.0 V phase command. It stopped during vector 1:

```text
STOP run=1 reason=current fault=1 travelDeg=-3.516 peakBoundA=0.364 encoderErrors=0
```

Subsequent telemetry confirms `drive=0 fault=1 lastStop=current`. The second
board remains unactuated. This verifies observed motor motion and a current
guard trip from telemetry, but NOT complete phase/motor acceptance. The current number is the
conservative `abs(iA)+abs(iB)` indicator, not a calibrated single-phase current.
V1 did not save the individual channel values at the trip, so the precise cause
is unresolved. The user's visual/audible observation and physical identity of
the first motor have been requested.

Prepared v2: reduce phase voltage from 1.0 to 0.5 V and add signed channel
currents, VIN, vector, timestamp and elapsed time to the stop record. Keep the
0.35 A current abort and all other guards unchanged. Not uploaded yet; batteries
must be disconnected before flashing. Existing boards still run v1, with 1410
latched off and 1430 idle.

User missed the physical movement and explicitly requested another test. No
visual/audible assessment or physical A/B label is established yet. Informed
the user that the repeat will use v2 at 0.5 V and requires disconnecting the
batteries before upload. No repeat command or reset has been sent. V2 host
control-flow checks and ESP32 compilation passed; artifacts are in
`/private/tmp/parallel-phase-startup-v2-build`. At the latest check, 1410 remained
latched off at 12.32 V and 1430 idle at 11.14 V. Give clear advance notice before
the next motor start so the user can observe it.

## User-authorized powered upload for the repeat

The user explicitly requested proceeding with batteries connected, noting that
prior uploads were also performed this way. This session authorization overrides
the usual battery-disconnection workflow for this repeat on the secured motors.
Sent `x` to both boards and verified `drive=0` before closing the v1 monitors.
Then began uploading v2 at 115200 baud with battery power present. Software
disable before/after upload does not establish GPIO states during reset.
Keep the mechanism clear, verify fresh post-upload status, and give advance
notice before sending the next bounded test command. The second motor's supply
remains near 11.14 V; no second-motor test is planned at that supply level.

## V2 upload and first complete phase cycle

Both powered uploads completed at 115200 baud with successful hash verification.
Transcripts: `phase-v2-upload-1410.log`, `phase-v2-upload-1430.log`. The new live
monitors are 99488 (1410) and 2453 (1430), writing `phase-v2-1410.log` and
`phase-v2-1430.log`. Prior phase logs remain intact. Both fresh boot states were
`run=0 drive=0 encoder=OK fault=0`. Voltages were 12.31 V and 11.11 V.

Gave clear advance notice and waited 5 seconds, then sent exactly one `w` to
1410. V2 recalibrated zero current to 1667.00 / 1667.13 mV and ran at 0.50 V.
The complete six-vector sequence finished in 3600 ms. Final encoder travel was
-28.059 degrees, peak conservative current bound 0.252 A, zero encoder errors,
and no fault. The motor remained disabled after `cycle-complete`.

First endpoint samples (just after disabling each vector, retaining the most
recent energized current sample):

| Vector | Travel (degrees) | iA (A) | iB (A) |
| --- | --- | --- | --- |
| 1 | -0.220 | +0.120 | -0.048 |
| 2 | -5.449 | +0.062 | +0.054 |
| 3 | -11.953 | -0.040 | +0.122 |
| 4 | -14.436 | -0.128 | +0.070 |
| 5 | -21.907 | -0.052 | -0.052 |
| 6 | -28.125 | +0.064 | -0.094 |

This passes the basic logged forward phase response on 1410, not full FOC,
bidirectional motion, continuous-duty, or bilateral/force validation. Current
values use the assumed shunt/amplifier gain and are not externally calibrated.
Requested the user's physical motor identification and noise/motion observation;
that reply is pending. Port 1430 remains `run=0 drive=0 fault=0`, with voltage
around 11.13 V. Restore adequate battery charge (or verify divider scale with a
meter) before testing that motor. No command has been sent to energize 1430.

User observation received (translated from Hungarian): "It worked without
problems." The first motor's basic phase check is therefore supported by both
telemetry and user observation. Physical left/right labels were not supplied;
identify it for now as the motor that just moved, port 1410.

Next requested physical action: swap the two battery packs between the boards,
leaving USB connected and both motors disabled. Compare the measured voltages
after the swap to see whether the low reading follows the battery, and use the
better-charged pack for the first phase test on port 1430. Wait for the user's
confirmation of the swap and fresh telemetry before sending any motor command.

## Battery swap and second motor phase test

The user confirmed swapping the batteries. Fresh logs showed 11.29–11.30 V on
1410 and 12.12–12.13 V on 1430, both disabled with healthy encoders and no fault.
The large voltage difference followed the packs. The same packs read about
0.17–0.18 V differently between boards; this suggests a board measurement-scale
difference or other measurement/load effects and requires a meter comparison
before treating VIN values as calibrated. Do not silently change VIN_SCALE.

Gave a 5-second advance warning, then sent exactly one `w` to 1430 through
monitor 2453. The first motor remained disabled. Fresh zero-current calibration:
1666.74 / 1667.38 mV, spans 10 / 11 mV. Run 1 at 0.50 V completed all six
vectors in 3600 ms, with no encoder errors or fault. Final net travel was
-16.458 degrees; peak conservative current bound was 0.241 A. Supply stayed
around 12.12 V. Latest state is `drive=0 fault=0 lastStop=cycle-complete`.

| Vector | First endpoint travel (degrees) | iA (A) | iB (A) |
| --- | --- | --- | --- |
| 1 | +5.405 | +0.121 | -0.047 |
| 2 | +3.384 | +0.133 | +0.065 |
| 3 | -1.121 | -0.055 | +0.099 |
| 4 | -8.416 | -0.103 | +0.055 |
| 5 | -11.887 | -0.047 | -0.049 |
| 6 | -17.314 | +0.057 | -0.101 |

The first vector aligns from an arbitrary starting pose, and the low-voltage
sequence includes unpowered gaps; net travel alone is not a torque comparison
or precision positioning test. Both boards now have a complete logged basic
phase cycle. First motor user observation was normal; the second motor's
visual/audible confirmation has been requested and is pending.

Pack locations after swap: lower-voltage pack on 1410, better-charged pack on
1430. Both motors are disabled. Further work remains: reverse/continuous
motion, per-assembly direction and FOC-zero calibration, pair MAC configuration,
radio/bilateral/fault checks, and later force tuning. Charge the lower-voltage
pack separately from the controller before subsequent paired testing.

## Second observation and calibration-reuse discussion

The user confirmed that the second phase test was normal too. Both basic phase
checks now have successful telemetry and user observations. Both boards remain
on phase firmware v2 with drivers disabled; no further motor commands sent.

Prepared a standalone `MKS_FOC_Startup_Calibration` candidate with nonblocking
forward/reverse field sweeps and electrical-zero estimation. It compiles but
has NOT been uploaded, run on hardware, or control-flow-tested. Its README
records validation still needed. Build: `/private/tmp/parallel-foc-startup-build`.

User asks how to avoid calibration for every assembly. Clarification requested:
are these the same physical motor/encoder assemblies as before, or new instances
of the same type? An unchanged assembly with identical phase mapping can reuse
its prior absolute-encoder electrical zero and direction. Same model alone is
not enough. Main firmware currently hardcodes the old pair's calibration and
selects it by board MAC; NVS currently stores only the tactile profile. A future
per-assembly calibration record/import would separate motor calibration from
board identity. No such persistence change has been implemented yet.

User clarified that these are NEW physical motor/encoder assemblies of the same
type. Their electrical-zero offsets therefore need individual establishment;
the old pair's values cannot be assumed transferable. Shared model parameters
(11 pole pairs, signal mapping, initial control profile) remain reusable.
The intended streamlined workflow is one commissioning/calibration per new
assembly, persistent motor-specific records, and automatic stationary current
zero sampling at startup. Recalibration is then needed only after relevant
mechanical/phase-mapping changes, data loss, or failed verification, not every
power cycle or ordinary firmware upload. The present diagnostic steps check
new cable/assembly quality and are not all recurring calibrations.


## Documentation decision: retain manual commissioning

The owner deferred automatic calibration storage and pairing. New assemblies
will continue through separate diagnostics, manually recorded measurements and
manual main-firmware configuration. The procedure is now documented in
the [manual commissioning guide](../new-system-commissioning.md),
with a [blank assembly record](../commissioning-record-template.md).
The [simplified workflow](../future-development.md) is future work. This decision
and documentation do not complete the outstanding FOC, radio or main-system tests.


## FOC commissioning resumed

Owner confirmed secured, free motors and charged connected batteries. Fresh
phase telemetry was disabled on both, with VIN about 12.53 V (A) / 12.35 V (B).
Both calibration-v1 builds were uploaded at 115200 baud with verified flash
hashes. The CAL newline was corrected and actual-sketch host control-flow tests
passed before upload. Calibration convention was checked against installed
SimpleFOC 2.4.0 source.

A's first 0.30 V alignment was rejected: forward -33.311 degrees, reverse
+25.298 degrees, return error -8.012 degrees, peak conservative current bound
0.257 A, no encoder errors. No accepted zero was recorded. Both outputs are off;
A has the expected validation fault latch. This suggests inadequate settling or
alignment torque, not a proven wiring fault. Logs: `foc-1410.log` / `foc-1430.log`.

Calibration v2 increases alignment Uq to 0.50 V, settle periods to 1500 ms,
sweeps to 2500 ms and middle hold to 700 ms (8.7 s nominal; 12 s timeout).
The conservative current bound becomes 0.60 A, below the old main's 0.70 A
boost ceiling; this bound sums absolute A/B currents and is not a calibrated
single-phase measurement. Final averaging remains the last 300 ms. Repeatability
and independent FOC checks remain mandatory before accepting a zero.


V2 A was also rejected: forward -28.784 degrees, reverse +28.630 degrees,
return error -0.155 degrees, pole estimates 12.507/12.574, peak bound 0.359 A,
no encoder errors. Return improved but the 11-pole-pair check did not pass.
Neither rejected trial produced an accepted zero.

V3 matches the main controller's PWM common-mode center (3.5 V, driver ceiling
7 V), retaining default Uq 0.50 V, the 0.60 A conservative bound and v2 timing.
The former low common-mode center was a diagnostic/main mismatch; its physical
contribution to the failed alignment is a hypothesis to test, not established.
Stopped-only `l`/`h` select Uq 0.50/0.70 V and invalidate the candidate; neither
clears a fault. `v` requires a valid RAM calibration and performs independent
FOC position moves: home, +8 degrees, home, -8 degrees, home, 1 s per stage.
It uses bounded PI voltage (P=5 V/rad, I=1 V/(rad*s), integral cap 0.20 V),
existing current/encoder/VIN/travel guards and a 3-degree endpoint tolerance.
It never starts automatically. The host checks also reject verification without
calibration and verify that a stuck verification movement stops with a fault.


## Accepted replacement-pair FOC calibration

At v3 Uq 0.70 V with the main's 3.5 V PWM center, both units passed the
calibration checks. The owner then rotated both disabled shafts approximately
a quarter turn, and both repeat measurements passed. Electrical-zero differences
were 0.003746 rad (A) and 0.004564 rad (B). For this supervised commissioning,
these are comfortably below a 0.10 electrical-rad repeatability check; this is
an engineering acceptance check, not a precision encoder calibration claim.

| Node | Direction | First zero (rad) | Repeat / adopted zero (rad) | Calibration peak bound |
| --- | --- | --- | --- | --- |
| A | CCW | 1.562826 | 1.566572 | 0.504 A |
| B | CCW | 4.839070 | 4.843634 | 0.473 A |

The latest candidate on each node then passed independent closed-loop FOC
position verification with `v` (five 1-second stages):

| Node | Home 1 | Positive target | Home 2 | Negative target | Home 3 | Peak bound |
| --- | --- | --- | --- | --- | --- | --- |
| A | +0.022 deg | +7.734 deg | +1.011 deg | -8.108 deg | -1.099 deg | 0.335 A |
| B | 0.000 deg | +7.427 deg | -0.088 deg | -7.888 deg | -0.242 deg | 0.311 A |

Both completed with fault=0 and zero encoder errors. The owner confirmed normal
motion without abnormal noise or strong jerking. Accepted values and raw result
fields are in `foc-candidates.json`; this file now marks calibration accepted.
VIN scales and current gain have not been externally meter-calibrated.

## New pair configuration and stronger boost preparation

Both station MAC arrays were updated in the main and link diagnostic, and the
main received the adopted zeros above (both existing CCW direction assignments
remain correct). A pre-change main snapshot is retained in
`MKS_Parallel_Mirror-before-new-pair.ino.txt`. The initial main build has
HANDOFF_MODE=false and autoStartEnabled=false; it needs explicit coordinated
arming and is not yet the final autonomous handoff build.

Boost now uses a 0.008 rad deadband, linear spring exponent, springK=4.50,
friction compensation 0.24 V and a 3.50 V command ceiling. Gentle/normal/strong
remain unchanged. Current and thermal envelopes remain unchanged for initial
hardware comparisons: boost 0.52/0.70 A, thermal 0.45/0.60 A. The actual spring
function's host checks passed bounds, symmetry, monotonicity and >3x requested
small-angle voltage. At 5/10/20 degrees the spring component changes from
0.154/0.292/0.527 V to 0.533/0.974/1.774 V before foldback, damping and integral.
This is a command comparison, not yet a measured force gain.


## Radio pairing and movement exchange

The updated link diagnostic was compiled and flashed to both nodes at 115200
with verified hashes. Both identified correctly and received fresh positions
(age normally 0–2 ms). The owner manually moved both shafts and confirmed the
sequence complete; each peer's remote position followed the other shaft, with
encErr=0. Recorded send failures were nonzero, so this is not a lossless-radio
claim. See `link-new-1410.log`, `link-new-1430.log` and `link-new-summary.json`.

B was deliberately reset while A logging continued. A reported six LOST samples
and then returned to link=OK with ageMs=1, validating loss detection and reception
recovery without motor actuation. Main motor-stop behavior is a separate check.
The link diagnostic still uses historical VIN_SCALE=8.5, so its boot readings
12.70/12.51 V are not directly comparable with the 8.29-scale calibration/main.

The compiled controlled main build is now being uploaded to both nodes. It uses
accepted motor constants, new MACs and the stronger boost curve, with automatic
start disabled. No bilateral actuation has yet been requested.


## Controlled main test and discovered runtime voltage clamp

The first controlled main was uploaded to both boards with verified hashes.
Both reported foc=1, armed=0, auto=0, matching protocol and peer readiness.
Gentle profile was selected on both, followed by an announced coordinated `a`.
The owner confirmed normal bilateral response. A-side `x` produced A manual stop
and B peer stop, both armed=0/auto=0/torqueV=0. Gentle current maxima were
0.486 A (A) and 0.477 A (B), with no thermal foldback.

The first increased boost was then tried. The owner reported working motion,
no jerking, but occasional oscillation absent in the earlier version. Both were
stopped before modifying the firmware. Logs: `main-new-1410.log` and
`main-new-1430.log`. Boost peaks were 0.650 A / 0.557 A, and requested torqueV
reached 3.5 V. Requested voltage did not prove applied voltage.

Source inspection found a concrete limitation in the previous main:
`driver.voltage_limit=ALIGN_VOLTAGE` (1 V) during `motor.init()`. Installed
SimpleFOC 2.4.0 `BLDCMotor::init()` clamps motor.voltage_limit to that driver
limit. The main subsequently restored only the driver's 7 V limit. The motor
therefore retained a 1 V internal limit despite larger requested torqueV.
This affected the pre-existing main too, not just the newly increased profile.

Controlled main v2 restores both limits with `configureRuntimeVoltageLimits()`
after initialization. It uses SimpleFOC's `updateVoltageLimit()` to update the
motor and internal controller limits together. Telemetry now includes `uqV`
(the enabled motor's computed q-axis voltage, not a physical voltage measurement)
and `motorLimitV`. This permits checking the previous hidden clamp in hardware.
Hard-current events now bypass the slew ramp and immediately set the command
to zero; the existing 0.70 A boost ceiling is unchanged.

To address the observed oscillation, boost keeps springK=4.50, 3.50 V ceiling,
0.008 rad deadband and linear exponent, but uses friction=0.16 V, positionI=0.08,
integralLimit=0.10 V, viscousDamping=0.140 and couplingDamping=0.100. The first
trial had 0.24/0.26/0.28/0.030/0.060 respectively. This is a candidate damping
adjustment awaiting physical validation, not a claimed oscillation fix.
Actual-source host checks pass runtime limit restoration, immediate hard-current
cut, spring monotonicity/symmetry/bounds and >3x small-angle boost command.
The v2 main compiled successfully and is being uploaded at 115200. It still
starts disabled and is not the final autonomous handoff configuration.


Controlled v2 uploaded with verified hashes to both nodes; no powered bilateral
test was started on it. Before proceeding to stronger actual drive, source review
also found that ESP32 HardwareSerial defaults to a zero-length TX software
buffer. A full status line can therefore block the control loop during UART
transmission. Controlled v3 sets a 2048-byte TX buffer before Serial.begin and
skips a periodic status line when less than 1024 bytes are free. `loopMaxUs`
records the largest loop gap since arm. This addresses a concrete timing issue;
its contribution to the reported oscillation has not been measured separately.
All force/damping/limit fixes from v2 are retained, with no current-limit increase.


## Corrected-limit / damped boost hardware trial

Controlled v3 uploaded to both nodes with verified hashes. Initial telemetry
showed motorLimitV=3.50, disabled outputs and the expected peer identities.
After announced arming, the owner tried the damped boost and reported improvement,
but insufficient force. The owner expects body mounting to constrain oscillation;
this remains an actual-mounting verification item, not an established guarantee.
Both were stopped with A manual stop / B peer stop, armed=0 and uqV=0.

The actual computed q-axis command exceeded the previous 1 V cap. Under load,
current foldback reduced the command, confirming that current limits now affect
available force. See `main-v3-summary.json` and both raw `main-v3-*.log` files.
The recorded control-loop maxima remain around 18 ms even with buffered serial;
this is not a certified hard realtime bound and UART buffering did not eliminate
all loop gaps. No thermal foldback was observed in this short trial.

The owner reports both motor/controller assemblies cold after the trial; no
numerical thermometer measurement was supplied. Controlled v4 increases only
boost current thresholds from 0.52/0.70 A to 0.75/0.95 A for the next short
supervised trial. Voltage, damping and the original thermal 0.45/0.60 A envelope
remain unchanged. Host regression checks pass; hardware results are pending.


## Higher-current boost trial (controlled v4)

V4 uploaded to both nodes with verified hashes. Both were initially disabled;
boost was selected and coordinated start was announced. After the brief user
comparison, A-side `x` stopped both nodes (manual/peer stop, armed=0, uqV=0).
The measured peak currents approached 1 A; finite control/sampling delay means
the software threshold is not an exact physical no-overshoot ceiling. Thermal
foldback remained inactive, while both current and speed foldback occurred.
See `main-v4-summary.json` and `main-v4-1410.log` / `main-v4-1430.log`.
The owner was asked to assess whether the force is now sufficient and whether
oscillation/jerking remains. That assessment is pending at this entry.


The owner reports v4 still too weak, with occasional separate clicks in either
motor. They explicitly defer further unloaded-oscillation tuning, expecting limb
mounting to constrain it. That expectation is not a verified stability result.
Both motors remain disabled while click/force causes are assessed. The owner was
asked how force is felt (shaft versus lever/disc and its effective radius) and
whether the latest trial left both motor/controller assemblies cold.

The current official [iFlight product listing](https://shop.iflight.com/ipower-motor-gm3506-brushless-gimbal-motor-w-as5048a-encoder-pro1155)
was checked: it lists a 12 V / 1 A load point with 600–1000 g*cm torque,
5.6 ohm internal resistance, 72 g mass and maximum power <=25 W. It does not
specify that 1 A as this firmware's measured phase-current limit, nor does the
maximum-power number establish continuous stalled or body-mounted operation.
The listed torque converts to approximately 0.059–0.098 N*m; force at a lever
is torque divided by effective radius. Actual prototype torque is not yet measured.


## Next force step: controlled v5

The owner clarified that force is felt directly at the shafts, without a lever,
and that both motor/controller assemblies remain cold after v4. They explicitly
asked to continue. V5 retains the damped profile's damping/integral/friction and
3.50 V ceiling, increases springK from 4.50 to 8.00, and sets boost soft/hard
current thresholds to 1.10/1.35 A. Original thermal limits remain 0.45/0.60 A.
This is a short supervised test step, not a validated continuous current rating.

At 5/10/20 degrees, the spring request is now 0.751/1.482/2.888 V before
foldback/damping/integral. Telemetry adds per-arm `hardCuts` and `speedCuts` event
counts to distinguish hard-current and hard-speed interventions from physical
clicks. Counter correlation still needs observation; clicks are not diagnosed.
The host checks and ESP32 compile passed. See `main-v5-manifest.json` and the
source snapshot for exact configuration and hashes. Uploads are in progress.


### Controlled v5: uploaded and short force trial completed

Both uploads completed successfully with flash hash verification at 115200 baud. Both nodes reported FOC ready, matching boost profiles, and disabled automatic start before the test. After a five-second advance warning, A received `a`; approximately 25 seconds later A received `x`. Both nodes confirmed disabled operation (A manual stop, B peer stop).

Measured current peaks were 1.304 A on A and 1.210 A on B. Neither node counted a hard current cut. A counted eight full speed cuts and B counted two. The thermal estimator began reducing output during the trial; this is a software current-based estimate, not a measured motor temperature. These observations do not establish whether any perceived click coincided with a cut. See `main-v5-summary.json` and the raw v5 logs. User assessment of force, clicks, and temperature is pending. Final autonomous handoff and body-mounted demonstration acceptance remain pending.


### User assessment after controlled v5

The user reports no noticeable heating. A lever approximately 25 cm long has now been fitted; oscillation was noticeably worse with this lever, and perceived force remains insufficient. This supersedes the earlier expectation that additional mechanical loading would necessarily suppress oscillation. No further actuation or strength increase was performed in response to this report. Actual torque and the safe continuous hardware maximum have not been measured. The previous v5 telemetry peaks represent approximately 94% of the configured 3.5 V Uq ceiling and 90–97% of the configured 1.35 A current cutoff, not percentages of validated hardware capacity.
