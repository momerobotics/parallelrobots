# Replacement-pair phase startup test

For the documented MKS ESP32 FOC Mega M0 + GM3506/AS5048A assemblies.
Adapted from the project's earlier `GM3506_MKS_Phase_Test`. This is a supervised,
short phase-vector diagnostic, not a bilateral controller or force profile.

Disconnect battery power before uploading. Always upload at **115200 baud**:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32:UploadSpeed=115200 firmware/MKS_Phase_Startup_Test
arduino-cli upload -p <port> --fqbn esp32:esp32:esp32:UploadSpeed=115200 firmware/MKS_Phase_Startup_Test
```

Startup leaves EN and PWM outputs disabled; it never runs a motor automatically.
No old MAC pairing or FOC zero calibration is used. No encoder OTP is written.
After upload, secure the motor housing and clear its motion path before restoring
its own protected 3S battery. Review VIN/encoder/current-zero status before `w`.

Serial commands at 115200 baud:

- `?`: status, without motion.
- `w`: recheck supply/encoder, recalibrate current offsets while disabled, then
  issue one sequence of six static phase vectors at a 0.5 V PWM command.
- `x`: stop and disable. A buffered stop takes priority over a buffered start.

Each vector is energized for 350 ms then disabled for 250 ms, for about 3.6 s
total. An overall 5 s software timeout is an additional bound. No automatic
repeat or restart. This checks phase response; it is not a constant-speed test.
Successful completion alone does not establish motion: inspect the encoder
endpoints, currents and the user's observation for each vector.

The controller stops and latches faults on encoder/parity/diagnostic errors,
out-of-range supply, excessive measured current, or more than 90 degrees net
travel. A reset clears faults but leaves the driver disabled. Investigate the
reason before another test. A plain manual stop does not clear a fault latch.

Initial test settings, not independently calibrated hardware limits:

- Supply window: estimated 11.1–13.0 V; historical VIN divider scale 8.29.
- Current conversion: 10 mOhm shunt, gain 50, 500 mV/A.
- Current abort: `abs(iA)+abs(iB) >= 0.35 A`, used as a conservative phase-current
  bound without assuming amplifier polarity. ADC readings are sequential and
  not PWM-synchronous; transient trips need investigation, not blind limit raises.
- Current-zero plausibility: both outputs 1400–1900 mV with <100 mV span during
  256 samples. Fresh offsets are captured immediately before every test.
- PWM: 40 kHz; phase command ceiling 0.5 V (v2).

Verbose serial output occurs with the driver disabled, including each vector
endpoint and final stop. The v2 stop record includes timestamp, vector, elapsed
time, both signed currents, current bound and VIN. Peak-current bound and encoder error count persist
through the run. These software checks do not constitute hardware protection
or guarantee a stop if firmware execution itself fails.

First hardware attempt (v1, 1.0 V): port 1410 moved about -3.516 degrees, then
tripped on vector 1 at a 0.364 A current bound, with zero encoder errors.
This does not identify which channel caused the trip or distinguish a transient
from sustained current. The complete six-vector test did not finish. V2 lowers
only the phase voltage to 0.5 V and improves stop telemetry; the 0.35 A abort
threshold remains unchanged. V2 was subsequently uploaded to both boards at
115200 baud. On port 1410 its first run completed all six vectors in 3600 ms,
with -28.059 degrees final travel, a 0.252 A peak current bound, no encoder
errors and no fault. The user confirmed normal operation without problems.
After swapping battery packs, port 1430 read about 12.13 V and also completed
all six vectors in 3600 ms with no encoder errors or faults: net travel
-16.458 degrees and peak conservative current bound 0.241 A. The second
motor's user observation is pending. Net travel from different starting poses
does not measure relative motor torque. Both basic logged phase tests are
complete; reverse motion and FOC calibration still remain.

Validation includes ESP32 core 3.3.11 / Simple FOC 2.4.0 compilation and host
simulation of startup, vector completion, stop priority and fault paths.
The driver interface follows
[SimpleFOC's standalone 3PWM documentation](https://docs.simplefoc.com/bldcdriver3pwm).
