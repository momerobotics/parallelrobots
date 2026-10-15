# Supervised legacy spring comparison

Purpose: test the user's original bilateral interaction intent: increasing angular separation produces increasing restoring effort on both participants.

The `l` command selects a separate, non-persistent `legacy` profile. It implements the supplied sketch's static spring law: zero within 0.01 rad, otherwise -2 V/rad times local-minus-remote angle, clipped to 3.5 V. No friction compensation or integral assistance is applied. Local damping is 0.02 V/(rad/s), with no relative-velocity damping. The boost profile remains available unchanged.

This is a controlled comparison, not a byte-for-byte port. Existing current protection (1.10 A soft / 1.35 A hard), thermal estimator, speed foldback, voltage slew, radio timeout, pairing, calibration and coordinated stop remain active. The original packet-driven alpha=0.1 angle filter is not reproduced; current radio timing and angle handling remain unchanged. Consequently, dynamic feel may differ from the original sketch even if the static spring law matches.

Automatic startup remains disabled. Each legacy arm session stops after 30 seconds in firmware and clears run intent. Start only after an advance warning, with the mechanisms secured on the bench. Confirm both nodes stop and record perceived proportionality, oscillation, clicks and temperature. Do not treat this build as accepted for body-mounted operation.

Validation: host checks passed for spring symmetry, bounds, deadband, the exact legacy slope, and existing hard-current cut behavior. ESP32 compilation passed and both uploads were hash-verified at 115200 baud. The bench trial was manually stopped before its 30-second deadline: A recorded 18 speed cuts and one hard-current cut (peak 1.377 A); B recorded 29 speed cuts and no hard-current cut (peak 1.152 A). Thermal scale stayed at 1.0 during recorded armed samples. Both nodes confirmed disabled after the stop. User assessment is pending; movement origin (manual motion versus self-oscillation) is not yet established. The firmware timeout was not exercised to completion. Raw log startup bytes include non-UTF-8 data; summary parsing replaces undecodable bytes and uses telemetry rows only.


## User assessment and damped follow-up

The user reports substantial self-oscillation, but a more natural buildup of opposing torque. Preserve the 2 V/rad static spring law. The follow-up changes only legacy damping: local damping 0.02 to 0.14 V/(rad/s), relative-velocity damping 0 to 0.10 V/(rad/s). Voltage/current limits and all other guards remain unchanged. The earlier section describes the first trial; the current source contains this damped follow-up. Hardware validation pending.


### Damped follow-up measurement

Both uploads were hash-verified. Trial stopped early after repeated speed cuts: A 19, B 17. Current peaks A 1.192 A, B 1.197 A; no hard-current cuts and no thermal foldback in recorded armed samples. Both nodes confirmed disabled. User assessment pending. Increasing damping alone has not eliminated speed-cut activity. One hypothesis for further investigation is the total-command slew limiter delaying the damping response; this is not yet established as the oscillation cause. Neither legacy trial is accepted for body-mounted use.
