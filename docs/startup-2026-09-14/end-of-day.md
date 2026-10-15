# End-of-day continuation baseline — 2026-09-14

**Final installed state:** The extra same-voltage SVPWM comparison completed (31 C to 32 C, approximately unchanged feel). The saved **knee10-no-filter SinePWM baseline has been restored on both boards**, with verified flash hashes and successful disabled startup. An interrupted upload was recovered after USB reconnection. Both received manual stop. No additional powered test followed restoration.

The saved SinePWM revision is **knee10-no-filter**. Work stopped after the final supervised two-minute bench trial. This is the saved continuation baseline, not a declaration of stable wearable or continuous operation. The operator deliberately allowed oscillation near the end; oscillation remains unresolved.

## Frozen configuration

- Main source: `../../firmware/MKS_Parallel_Mirror/MKS_Parallel_Mirror.ino`.
- Exact source copy: `MKS_Parallel_Mirror-force-knee10-no-filter.ino.txt`.
- Source SHA-256: `26f5824170a1fc86ad7c7f7d2147a400e827b01a754592b72c130de0106e7f2f`.
- Application binary SHA-256: `f696ffd05f6ebea5966a78555e57d992fd98796530ec3d9135151db3334a812d`.
- Flash files and checksums: `closing-build-knee10-no-filter/`.
- Full settings and upload evidence: `main-force-knee10-no-filter-manifest.json`.

| Setting | Value |
| --- | --- |
| Static force curve | 68% at 10 degrees, then gradual linear rise to 100% at 60 degrees |
| Percentage meaning | Fraction of requested voltage cap, before protection scaling; not measured torque |
| Deadband | 0.01 rad |
| Voltage cap / modulation | 5.5 V Uq / centered SinePWM |
| Instantaneous soft / hard current thresholds | 1.75 / 2.00 A |
| Thermal-estimate soft / hard thresholds | 0.90 / 1.10 A, 45-second EWMA |
| Magnitude attack | 80 V/s; immediate release |
| Experimental angular-error filter | Removed |
| Automatic start / handoff mode | Off / off |
| Supervised force session | 120 seconds |
| Upload speed | Always 115200 baud |

The 25-degree rollback was never uploaded. The 10-degree curve and wider thermal-estimate range were retained; only the experimental error filter was removed. No SpaceVectorPWM change was made.

## Closing trial

Both nodes armed once and stopped on their timers. Reported current peaks: A 1.843 A, B 1.832 A; both reached 5.5 V. No hard-current cuts. Thermal scaling reached minima A 0.62 / B 0.65. Speed-cut events: A 169 / B 122; the operator deliberately allowed oscillation near the end. These counts are not oscillation frequencies.

Motor probe: 28 C baseline, 30 C and 34 C during the session, and **35 C final**, explicitly confirmed by the operator after correcting an intervening 30 C report. This was one motor-surface probe, not winding or both-motor temperature.

See `main-force-knee10-no-filter-summary.json` and the two matching serial logs. Following the timed stop, both boards received explicit manual stop and confirmed `armed=0 auto=0 stopLatch=1 uqV=0`. Serial monitors were then closed.

## Resume without losing this baseline

1. Keep this source snapshot and the archived binaries; no routine re-upload is needed while these boards retain the installed firmware. If an upload becomes necessary, include `UploadSpeed=115200` in the FQBN.
2. Use the [manual commissioning guide](../new-system-commissioning.md) for any new board or motor/encoder assembly; these calibration constants belong to this replacement pair only.
3. Connect monitors at 115200 baud with DTR/RTS off. Confirm node identities, FOC readiness, fresh peer packets, disabled motors and adequate battery supply. Final boot voltage estimates were A 11.34 V / B 11.17 V; they are not live or meter-calibrated readings. Check/charge batteries before resuming. The final restoration check booted on USB only (about 1.06 V reported), which latches low-battery inhibition and a reduced runtime cap. Connect the motor supply and restart both boards before the next supervised session so the firmware resamples boot voltage; merely attaching the battery after a USB-only boot does not refresh this reading.
4. After a reboot, select `f` on each board while disarmed. The force selection is not saved to NVS. Confirm both report `profile=force`; startup defaults must not be assumed to be force.
5. For a supervised bench trial, secure the assemblies, use the motor-temperature probe, and place both mechanisms at their intended corresponding neutral positions. Send each serial command separately: the parser discards the rest of a buffered batch.
6. `a` on one node starts the coordinated session and redefines each local zero. `x` stops; `s` selects strong and is **not** a stop command. Force operation times out after 120 seconds. The external probe does not control firmware; the prior 45 C operator stop threshold remains a trial procedure, not a validated winding rating.
7. Finish with `x`, verify both disabled, then disconnect motor batteries for storage.

## Deferred

No further tuning today. Retain oscillation, current/voltage measurement calibration, powered main-firmware link-loss acceptance, sustained thermal characterization and on-body acceptance as open items. A separate SpaceVectorPWM comparison may be considered later, beginning at the same limits and preserving this baseline. Automatic commissioning remains deferred.
