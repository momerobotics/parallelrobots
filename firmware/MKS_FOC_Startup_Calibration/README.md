# FOC startup calibration and verification

V3 is commissioned on the September replacement pair: both motors passed
repeat calibration from changed starting positions and independent two-direction
FOC position checks. See the [session record](../../docs/startup-2026-09-14/README.md)
and its `foc-candidates.json` for accepted values. Nothing starts automatically;
no NVS or OTP data is written.

## Commands

- `c`: bounded calibration sweep and electrical-zero candidate in RAM.
- `v`: independent FOC moves using a valid RAM candidate: home, +8 degrees,
  home, -8 degrees, home; 1 second per stage, 3-degree endpoint tolerance.
- `l` / `h`: select 0.50 / 0.70 V alignment while stopped; invalidates calibration.
- `x`: stop. A latched fault remains until reset after reviewing its cause.
- `?`: status.

Default alignment is 0.50 V. The September assemblies required `h` (0.70 V)
for successful calibration. Match the main controller's 7 V driver ceiling and
3.5 V PWM center. The calibration settles for 1500 ms, sweeps one electrical
revolution in 2500 ms, holds 700 ms, returns in 2500 ms and holds 1500 ms.
It averages the last 300 ms. Total nominal time is 8.7 s; timeout is 12 s.

Current abort uses abs(iA)+abs(iB) at 0.60 A. This is a conservative indicator,
not externally calibrated phase current. Encoder health, 90-degree net travel,
VIN 11.1–13.0 V and current-zero plausibility are also guarded. Voltage selection
does not increase those limits. Verification uses bounded PI voltage with
P=5 V/rad, I=1 V/(rad*s), integral cap 0.20 V and the selected voltage ceiling.

Accept electrical zero only after repeatability and independent `v` checks,
with normal physical motion confirmed. `CAL valid=1` alone is insufficient.
The older 0.30/0.50 V trials were rejected; their records remain in the session log.

The actual-sketch [host tests](../../tests/foc_startup_host/README.md) pass;
ESP32 core 3.3.11 / Simple FOC 2.4.0 compilation and uploads succeeded.

```sh
arduino-cli compile --fqbn esp32:esp32:esp32:UploadSpeed=115200 firmware/MKS_FOC_Startup_Calibration
```

Always upload at 115200 baud. Per-assembly measurements must be recorded and
manually integrated into the main; there is no automatic persistent calibration.
