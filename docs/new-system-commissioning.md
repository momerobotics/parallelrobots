# Manual commissioning of a completely new system

This is the current workflow: separate diagnostic sketches, manual recording of
each assembly's measurements, and manual configuration of the main firmware.
The proposed single-firmware commissioning/storage system is a
[future development direction](future-development.md), not an existing feature.

## 0. Scope and current validation boundary

This guide applies to two MKS ESP32 FOC **Mega** single-motor boards, two iPower
GM3506 24N/22P motors (**11 pole pairs**), AS5048A SPI absolute encoders and
separate protected 3S batteries. The older non-Mega board has a different pinout.
The [reproduction guide](reproducibility.md) contains hardware and software references.

The 2026-09-14 replacement pair passed encoder, idle-current, six-vector motor,
repeat FOC calibration and independent +/-8-degree FOC checks on both nodes.
The owner confirmed normal physical motion. Radio pairing, bidirectional position
exchange and observed peer loss/recovery also passed. Main force and final
handoff acceptance are in progress; see the [live session record](startup-2026-09-14/README.md).

The main and link sources now use this pair's station MACs. The main has the
accepted zeros A=1.566572 rad and B=4.843634 rad, both CCW. These are specific
to the recorded assemblies, not values to copy to another new pair. The
standalone calibration v3 has passed control-flow and hardware commissioning.

## 1. Hardware assembly and inspection

1. Label the two board/motor/encoder assemblies A and B. Label batteries separately
   so swapping a pack does not change the motor's identity. Record connector orientation.
2. With all power disconnected, check connector polarity and continuity against
   the actual board and encoder pin labels. Do not infer functions from wire colors.
3. Check that motor phase wires do not short to the housing or encoder supply.
   Compare A–B, B–C and C–A winding resistance with a meter; record the readings.
   Open circuits or markedly unequal values need investigation. Do not assume
   the product listing's resistance is a measured phase-to-phase value.
4. Secure the motor housings and leave the rotating parts unobstructed. Perform
   initial tests off the body, with no linkage pressing against a person.
5. Connect the encoder and motor cables. Never rearrange connector pins while powered.
6. Each board receives its own protected 3S pack; do not parallel the two packs.
   Disconnect a battery from the board before charging it.

| Signal | ESP32 GPIO | Note |
| --- | ---: | --- |
| Motor phases A/B/C PWM | 32 / 33 / 25 | Onboard inverter M0 |
| Motor enable | 12 | LOW disables the inverter in these sketches |
| Encoder SCK / MISO / MOSI / CS | 18 / 19 / 23 / 5 | SPI mode 1, 1 MHz |
| Current sense A / B | 39 / 36 | ADC1 inputs |
| VIN sense | 13 | ADC2; unavailable for dependable continuous reads with Wi-Fi |

The table is a logical signal map, not a physical connector drawing. Confirm
the physical header pin order on the actual hardware. The old boards' onboard
RGB LED has a known wiring defect; use telemetry rather than the LED as proof
of startup or motor-disable state.

## 2. Start an assembly record and identify USB ports

Copy [commissioning-record-template.md](commissioning-record-template.md) into a
new dated pair directory. Keep the original template blank. Record every firmware
revision, physical assembly, port, MAC, battery, measured zero and acceptance result.

Run the following from the `parallelrobots` repository root:

```sh
arduino-cli version
arduino-cli core list
arduino-cli lib list
arduino-cli board list
```

The original handoff was tested with Arduino CLI 1.4.1, ESP32 core 3.3.10 and
Simple FOC 2.4.0. The September diagnostics were built/run with CLI 1.4.1,
core 3.3.11 and Simple FOC 2.4.0. Record which set is actually used; do not silently
upgrade the environment. The new main-controller build must be compiled and
checked in that same recorded environment before release.

If necessary connect one USB board at a time to establish the port mapping.
`Unknown` in `board list` does not by itself mean that the ESP32 is defective;
use the explicit `esp32:esp32:esp32` target. Ports can change when cables move.
Use the **station MAC** printed by the startup sketch as the stable board identity.

Set these shell variables to the ports you just identified. These values are
examples from the September session, not universal addresses:

```sh
PARALLEL_PORT_A=/dev/cu.usbserial-1410
PARALLEL_PORT_B=/dev/cu.usbserial-1430
PARALLEL_LOG_DIR=docs/commissioning/new-pair
mkdir -p "$PARALLEL_LOG_DIR"
```

Choose a distinct log directory for each assembly session.

## 3. Upload and logging procedure used at every stage

**Always upload at 115200 baud**, including a first upload to a blank board.
Higher rates have repeatedly failed. Use
`--fqbn esp32:esp32:esp32:UploadSpeed=115200`, or Arduino IDE Upload Speed 115200.
Changing only the serial monitor rate does not change the upload rate.

For an unknown newly wired assembly, start with battery power disconnected and
USB connected. In the September supervised session the owner explicitly chose
powered uploads after disabled outputs and secured motors had been checked.
That session decision does not prove GPIO states during reset or establish a
general powered-upload guarantee for a different assembly.

Before every upload: stop any running test, verify both outputs disabled, close
the serial monitor on the target port, and confirm the physical target. Build
after editing the sketch; do not upload a stale binary from a different stage.
Check the upload's success and flash verification messages, then reopen logging.

Example for the startup encoder diagnostic:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32:UploadSpeed=115200 firmware/MKS_Startup_Diagnostic
arduino-cli upload -p "$PARALLEL_PORT_A" --fqbn esp32:esp32:esp32:UploadSpeed=115200 firmware/MKS_Startup_Diagnostic
arduino-cli upload -p "$PARALLEL_PORT_B" --fqbn esp32:esp32:esp32:UploadSpeed=115200 firmware/MKS_Startup_Diagnostic
```

For later stages substitute the sketch directory from the table below in both
the compile and upload commands. Run two monitors in separate terminals, with
different log filenames for each stage and board:

```sh
arduino-cli monitor -p "$PARALLEL_PORT_A" --config baudrate=115200,dtr=off,rts=off --timestamp | tee -a "$PARALLEL_LOG_DIR/encoder-A.log"
```

Use a second terminal for B with `$PARALLEL_PORT_B` and `encoder-B.log`. `Ctrl-C`
closes the monitor; it is **not a motor stop command**. Send the sketch's `x`
command first when it supports motor drive. Check that fresh lines are actually
being written before asking the operator to move a shaft or starting a test.
After an app/terminal interruption, recover and verify logging before continuing.
Use append/new files so previous evidence is not erased.

| Stage sketch | Commands | Motion |
| --- | --- | --- |
| `MKS_Startup_Diagnostic` | `r` resets counters/travel; `?` status | Never |
| `MKS_Current_Sense_Diagnostic` | No interactive commands | Never |
| `MKS_Phase_Startup_Test` v2 | `w` one cycle; `x` stop; `?` status | Only on `w` |
| `MKS_FOC_Startup_Calibration` v3 | `c` alignment; `v` FOC check; `l/h` voltage; `x` stop; `?` status | Explicit `c` or `v` only |
| `MKS_Bilateral_Link_Test` | No interactive commands | Never |
| `MKS_Parallel_Mirror` development | `a` coordinated start; `x` stop; `g/n/s/b` profiles; `c/z` calibration/pose zero | Can actuate; configure first |

Commands differ between sketches. In the main sketch `s` means the **strong
profile**, not stop. Send commands separately; its parser discards additional
characters already queued after the first command.

## 4. Encoder check and MAC capture

Load `MKS_Startup_Diagnostic` on both boards. Confirm `motor=DISABLED`, a real
station MAC and `encoder=OK`. Copy each MAC into the assembly record together
with the matching physical motor and port.

1. With the shaft stationary, record a baseline and reset counters with `r`.
2. Slowly rotate one shaft in both directions, then the other. Where the mechanics
   allow, include a full revolution and crossing through 0/360 degrees.
3. Confirm corresponding `angleDeg`/`travelDeg` changes, stable stationary readings,
   and no increments in `badFrames`, `parityErrors`, `errorFlags` or `unhealthySamples`.
4. Gently move the cable/strain relief to check for intermittent contact without
   unplugging live connectors. Record this separately from the rotation test.

Expected diagnostic flags: `OCF=1`, `COF=0`, `compHigh=0`, `compLow=0`.
All-zero SPI data is not a pass: the diagnostic also requires OCF. A full motion
trace is required; final net travel near zero does not prove the shaft never moved.
The sketch samples approximately 100 Hz and reports 2 Hz, so counters provide
additional evidence beyond the displayed snapshots. A failed sensor/cable check
must be resolved before energizing a motor.

## 5. Current-sense zero: USB, then actual motor supply

Load `MKS_Current_Sense_Diagnostic`. With outputs disabled, log at least a short
stationary baseline on USB. Then connect the actual test battery and log another
baseline. The sketch cannot identify battery presence because it does not report VIN.

It prints `rawA,mvA,noiseA,ampsA,rawB,mvB,noiseB,ampsB`. Save its startup zero offsets.
The September pair read about 1.661 V on USB and 1.667 V with batteries connected;
these are examples, not calibration values to copy. Look for stable, non-saturated
signals. A changed offset after changing supply can produce a nonzero displayed
`amps` value even though the motor is disabled. Remeasure zero under the actual
test supply; do not treat the USB zero as a permanent motor calibration.

This is an offset/stability check. It does not calibrate shunt gain, verify
current-channel polarity under drive or establish accurate phase-current limits.

## 6. Check VIN and power readiness

Load `MKS_Phase_Startup_Test` v2, which reports VIN while idle and leaves drive
disabled. Confirm `run=0 drive=0 fault=0 encoder=OK`. Restore the test battery
if it was disconnected for upload; keep the motor secured and clear.

Compare the printed VIN against a multimeter at the board's power-input terminals.
Record both measurements for each board. With no motor battery, a reading near
1.06 V has been observed; it is not a real usable battery supply.

The phase test permits an estimated 11.1–13.0 V. Start with adequately charged
3S packs, not a reading barely above the cutoff. If two readings differ, stop
both motors and swap the packs to see whether the difference follows the pack.
The September readings also showed about 0.17–0.18 V board-to-board difference
for the same pack; a meter comparison remains necessary.

For a proportional divider correction, an initial estimate is
`new_scale = old_scale * meter_voltage / reported_voltage`. Check it at multiple
3S supply levels, rather than treating one point as a complete calibration.
Record the accepted scale and residual error for each board.

Current source defaults differ: phase/calibration/main sketches use 8.29; the
older link diagnostic uses 8.5. Apply verified values in every relevant build
before comparing VIN across sketches. The main sketch currently has one global
`VIN_SCALE`, not a per-board table. If boards need different scales, the present
manual approach requires separately labelled builds with their own scale; keep
MAC pairing, protocol and control settings consistent. Do not claim a single
binary already supports two independent VIN corrections.

Main/link firmware reads VIN before Wi-Fi starts because GPIO13 is ADC2. Its
`bootVin` and battery percentage are not live battery monitoring.

## 7. Short phase test, one motor at a time

1. Verify a fresh log, acceptable VIN, healthy encoder, `drive=0` and `fault=0`.
2. Tell the operator which motor will move and give a clear 5-second countdown.
3. Send **one `w`** to that board. Do not start both at once.
4. Observe the small stepped movement. Wait for `reason=cycle-complete` and
   verify `drive=0 fault=0`, with zero encoder errors.
5. Record all six endpoints, current indicator, VIN, stop reason and the operator's
   observation. Repeat on the other board only after the first result is reviewed.

V2 uses six 0.5 V static phase commands: 350 ms energized, 250 ms disabled per
vector, 3.6 s total, with an additional 5 s software timeout. It recalibrates
current zero before each run. It stops on encoder, supply, current or travel faults.
`abs(iA)+abs(iB)` is a conservative current indicator, not a calibrated reading
of one phase. Its abort threshold is 0.35 A.

The earlier 1.0 V attempt stopped during vector 1 at an indicator of 0.364 A.
Lowering the test voltage to 0.5 V allowed both six-step tests to complete without
raising the current threshold. A current trip alone does not identify a bad motor
or justify raising limits. Save the evidence and investigate before restarting.
`x` disables outputs but does not clear a latched fault; reset only after review.

Small initial alignment movement and differing net travel are possible because
starting rotor poses differ. This test is not a torque comparison, continuous
rotation acceptance or FOC calibration. Do not use the old root-level open-loop
sketch unchanged: it still assumes 7 rather than 11 pole pairs.

## 8. Establish the motor/encoder calibration

V3 has passed the actual-sketch host checks, both hardware calibration repeats
and independent FOC position verification on the September pair. For each new
assembly, repeat the following manual procedure and record its own results.
Do not accept a zero solely because `CAL valid=1` was printed.

1. Record the physical motor/encoder identity and unchanged phase-to-terminal order.
2. Upload the approved calibration revision using section 3; verify disabled startup,
   fresh logging, charged supply and encoder health.
3. While stopped, select `h` for the 0.70 V alignment used successfully on this
   pair (`l` selects 0.50 V). Voltage changes invalidate a candidate and do not
   clear faults. Announce the movement and send `c` on one board only.
4. V3 holds/sweeps/returns the field in 8.7 s, with a 12 s timeout. Default Uq
   is 0.50 V; the September pair required 0.70 V. Its 7 V driver ceiling and
   3.5 V PWM center match the main controller. Current and encoder checks run
   during movement; the conservative current abort bound is 0.60 A.
5. Require `reason=calibration-complete fault=0` and `CAL valid=1`. Record `direction`,
   `zeroE` in **radians**, forward/reverse travel, both pole-pair estimates, return
   error, sample count, current peak, supply and exact firmware version.
6. Repeat from another free starting pose. Compare zero angles circularly modulo
   2π, not by naive subtraction across 0/2π. Record both values and their difference.
   The September pair differed by 0.003746/0.004564 electrical rad, comfortably
   within a 0.10 rad engineering repeatability check. Record the criterion and
   result for each new assembly; coarse sweep checks alone are insufficient.
7. Send `v` after a countdown to verify FOC position control using the RAM
   candidate: home, +8 degrees, home, -8 degrees, home, 1 s each. Require
   `VERIFY valid=1`, `reason=verification-complete`, no faults and normal physical
   motion; each endpoint must be within 3 degrees. Save the verified zero and
   direction in the assembly record and main source. V3 writes no NVS/OTP data.

The calibration checks opposite travel directions, both sweeps consistent with 11
pole pairs within 0.5 electrical rad, return error below 2.5 mechanical degrees
and at least 50 final averaging samples. Passing these tests does not by itself
prove an accurately settled rotor at the electrical zero.

**Existing main `c` caveat:** `calibrate()` currently forces `Direction::CCW`,
uses `CALIBRATION_ALIGN_VOLTAGE=1.5`, and calls blocking `motor.initFOC()`. It is
not automatic direction discovery, and the main loop's current monitoring is
not serviced during that blocking call. Setting only `HANDOFF_MODE=false` is
therefore insufficient as a generic new-assembly calibration recipe.
Do not use it to bypass this validation gate. `z` changes the interaction's
mechanical starting pose, not the motor's electrical calibration.

## 9. Configure both station MAC addresses and test the radio

Update **both** arrays in **both** of these files, keeping A/B consistent:

- [`MKS_Bilateral_Link_Test.ino`](../firmware/MKS_Bilateral_Link_Test/MKS_Bilateral_Link_Test.ino)
- [`MKS_Parallel_Mirror.ino`](../firmware/MKS_Parallel_Mirror/MKS_Parallel_Mirror.ino)

For the September pair only, the recorded mapping is:

```cpp
static const uint8_t MAC_A[6]={0x30,0xC9,0x22,0x5F,0x5E,0x2C};
static const uint8_t MAC_B[6]={0x88,0x57,0x21,0x19,0xD5,0x70};
```

For another pair substitute its measured station MACs. Use the printed byte order;
do not reverse it or substitute a Bluetooth/soft-AP address. The same two arrays
go into the builds for both boards: each board recognizes itself and selects the
other as its peer. There is no automatic discovery/pairing in the current version.

Load the updated link diagnostic on both boards. It never actuates a motor.
Require correct A/B identity, `ESP-NOW: OK`, `AS5048A: OK`, increasing `rx`,
`link=OK`, fresh `ageMs` comfortably below 250 ms and no growing `encErr`.
Rotate each shaft and verify that the other board's `remote` follows it. The raw
encoder direction here need not match the later direction-normalized main coordinate.
Disconnect/reset one peer and confirm link loss, then restoration of reception.
This validates communication, not the main controller's motor-stop behavior.

Radio testing can be completed before section 8. Main motor control requires
accepted calibration and independent verification. Link packets and main-controller protocol-7 packets are different;
one board on each sketch is not a compatible pair.

## 10. Integrate accepted values into a controlled main build

Do this only after both calibrations have been accepted. First save the original
source/firmware revision and the assembly record. Never guess a missing zero.

| Main source item | Manual commissioning setting |
| --- | --- |
| `MAC_A`, `MAC_B` | This pair's station MACs |
| `ZERO_ELECTRIC_A`, `ZERO_ELECTRIC_B` | Accepted motor-specific radians |
| `POLE_PAIRS` | 11 for this documented variant |
| `motor.sensor_direction` in `setup()` | Accepted direction for the selected node |
| `VIN_SCALE` | Verified scale for the board/build |
| `HANDOFF_MODE` | `false` for initial controlled checks |
| `USE_STORED_FOC_CALIBRATION` | `true` only with accepted constants installed |
| Initial `autoStartEnabled` | `false` on **both** boards for this stage |

The existing `setup()` assigns CCW to either node. If an accepted direction is
different, manually implement the appropriate A/B selection at that assignment;
changing only the zero constants is insufficient. Its separate `calibrate()`
function also assumes CCW; do not invoke that command for an incompatible assembly.
The September pair uses CCW on both nodes; its accepted zero/MAC values are
already integrated. Repeat these configuration checks for every new assembly.

The name `USE_STORED_FOC_CALIBRATION` refers to **source-code constants**, not
automatic persistent calibration. NVS currently stores the selected tactile
profile only. Record motor calibration independently of board identity so it
can be restored after a compatible board replacement.

Compile the main sketch, flash the appropriate build to each board at 115200,
and verify `radio=OK`, `storedFOC=OK`, `currentSense=OK`, `foc=1`, `armed=0`,
`auto=0`, correct MAC/node and matching protocol. `storedFOC=OK` alone does not
prove the copied electrical zero is physically correct.

Send `g` separately on both nodes for the gentle profile; check matching profile
telemetry. From this known stopped state, an `a` command can request coordinated
start and enable **both** motors. Announce this before sending it. Use small free
movements, verify response in both directions, then send `x` and confirm both
nodes report `armed=0` and zero torque command. Inspect current, speed, thermal
scales and link age along with the operator's observations.

## 11. Final handoff build and acceptance

After controlled tests, restore the intended handoff settings on both builds:
`HANDOFF_MODE=true`, `USE_STORED_FOC_CALIBRATION=true`, initial
`autoStartEnabled=true`, protocol 7 and the agreed handoff profile. Preserve the
new MACs, accepted zero/direction and verified voltage scaling. The current
handoff profile is boost; do not confuse the 0.5 V phase test with its output.

The handoff build can start automatically after peer synchronization and the
5-second startup delay. Keep the shafts free during startup; use the operator
guide's 8-second wait before handling the mechanism. Observe both logs before
standalone use. Record and verify:

- repeated simultaneous and staggered startup, with correct node identification;
- mirrored response and resistance in both directions, followed by coordinated stop;
- main-controller link timeout: remove/reset one peer and confirm the other stops;
- protocol-7 runtime fault latch: restoring radio alone does not restart motion;
- deliberate restart with both units and the operator procedure;
- 10-minute handoff session cutoff;
- operation on two charged, separate batteries with USB disconnected;
- measured heating, current behavior and the intended mechanical motion range;
- perceptible feedback in the actual mounting used for the demonstration.

The September pair has not yet completed these checks. The original weak
on-body feedback report remains an open acceptance issue. A working encoder or
phase test is not evidence that the final haptic force is sufficient.

Use [HANDOFF.md](HANDOFF.md) for operator steps,
and the [prototype checklist](prototype-checklist.md) for remaining limits.

## 12. Archive, maintenance and troubleshooting

Save source snapshot/revision, build versions, uploaded binaries or their hashes,
MAC-to-motor mapping, connector/phase map, raw logs, accepted calibration and
all pending checks. A normal firmware update need not repeat motor calibration
when the same motor/encoder assembly and phase mapping remain unchanged.
New physical motors/encoders require individual values; the same product type
alone does not establish the same electrical zero. Recheck after encoder/magnet
movement, phase-order changes, repair or failed verification.

| Symptom | First check |
| --- | --- |
| Upload fails | Close monitor, confirm port/cable, keep UploadSpeed=115200 |
| Blank app/monitor after interruption | Verify files, restart logging, prove fresh data before repeating motion |
| Encoder fixed at zero / CHECK | Connector pin map, supply, CS/MISO and OCF; no drive until resolved |
| Unknown node / radio failure | Station MACs in both arrays of both sketches; channel and sketch compatibility |
| Current trip | Save signed channels, voltage, peak and stop reason; review before reset |
| Supply near cutoff | Charge/swap battery, compare with a meter; do not defeat cutoff |
| No response after main `a` | Local/peer FOC readiness, profiles, link age, boot VIN and stop state |
| Calibration command locked | Handoff mode intentionally locks `c` and `z`; use the documented commissioning stage |
| Weak haptic feedback | First verify commissioning; then inspect command/current/speed/thermal limits under actual mounting |
