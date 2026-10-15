# Supervised force trial

## User intent and interpretation

The devices couple the same body joint on two participants. Greater relative joint displacement should produce greater reciprocal restoring torque. The user reports that synthetic damping obscures this signal by adding resistance that feels similar to the other participant resisting. Defer unloaded-oscillation tuning and evaluate stronger coupling in a controlled bench trial.

Both boards' telemetry is logged, including angle, velocity, Uq command, phase-current readings, peak current, guard scales and event counts. Status snapshots are approximately 2 Hz, not a high-rate waveform capture. Peak-current and event counters accumulate between snapshots; they cannot time-correlate every click or reconstruct oscillation. No direct force or torque measurement is available.

## Force profile

Select `f` while disarmed. Static spring gain is 4 V/rad, twice the supplied historical sketch. Deadband is 0.01 rad; there is no friction compensation, integral assistance, local damping or relative-velocity damping. Requested Uq is capped at 5.5 V, further limited to half the boot-estimated supply to stay inside the centered SinePWM phase range. The phase-voltage range is expanded to the boot-estimated supply only for this profile; leaving it restores the original 7 V range and 3.5 V motor ceiling.

The 1.10 A soft / 1.35 A hard measured-current limits, thermal estimator, speed foldback, slew limiting, radio timeout, coordinated stop and 30-second firmware trial timeout remain. This is maximum-voltage-range exploration under current limits, not a validated hardware maximum or a continuous/body-mounted rating. Higher voltage may not increase torque where current foldback already dominates.

Profile changes require disarmed operation. Force is not persisted, and boot automatic start remains disabled. Speed-cut counts alone do not terminate a supervised trial; evaluate them alongside current, communication and user observations.

## Validation

Host checks cover exact linear spring gain, symmetry, deadband, output bounds, restoration of voltage limits, half-supply clipping, return to the original phase range and immediate hard-current removal. Compilation and hardware results pending.


## Hardware result

Both uploads were hash-verified at 115200 baud. Both nodes reported the force profile, 5.50 V motor limit and disabled automatic startup before arming. After the advance warning, both ran until their 30-second firmware deadlines; both reported `supervised trial complete`, disabled motors and cleared automatic run intent. A redundant manual stop was then sent.

{
  "1410": {
    "armed_samples": 60,
    "peak_current_a": 1.353,
    "max_abs_uq_v": 3.365,
    "hard_cuts": 1,
    "speed_cuts": 18,
    "min_thermal_scale": 1.0,
    "min_current_scale": 0.3,
    "final_armed": 0,
    "timer_stop_observed": true
  },
  "1430": {
    "armed_samples": 60,
    "peak_current_a": 1.287,
    "max_abs_uq_v": 3.764,
    "hard_cuts": 0,
    "speed_cuts": 12,
    "min_thermal_scale": 1.0,
    "min_current_scale": 0.43,
    "final_armed": 0,
    "timer_stop_observed": true
  }
}

User assessment of perceived force and temperature is pending. Current foldback occurred, but the thermal estimator did not reduce output in recorded armed samples. Maximum observed Uq is a command value, not a voltage or torque measurement.


## Small-angle force follow-up

The user reports that force remains far too weak and clarifies the application: approximately 180 degrees total joint travel, with 10–30 degree inter-participant differences needing clear perception. Preserve a linear restoring law and zero added damping; raise the force profile spring gain from 4 to 10 V/rad. Before current, speed, thermal and slew restrictions, this requests 1.745 V at 10 degrees, 3.491 V at 20 degrees and 5.236 V at 30 degrees. The 5.5 V ceiling is reached near 31.5 degrees instead of 78.8 degrees. Current limits remain 1.10/1.35 A; this increases small-angle demand, not the validated maximum torque. Approximate anatomical travel is not treated as a calibrated software end stop. Compilation and hardware trial pending.


### Small-angle trial result

Both uploads verified at 115200 baud. A completed its 30-second deadline and B stopped on peer intent. Both motors disabled. No hard-current cuts or thermal foldback were recorded. Current peaks A 1.346 A, B 1.301 A. Current and speed foldback still occurred; increased static demand does not imply proportionally increased delivered torque. See `main-force-small-angle-summary.json` for peaks and sampled angular bins; those bins include moving samples and are not static torque measurements. User assessment pending.


## User assessment and current ceiling review

The user reports no noticeable heating but substantial oscillation even while restrained; the oscillation creates a false sensation of the other participant moving. The historical sketch felt more direct. The user asks whether doubling the 1.35 A current threshold to 2.70 A can be tested and where the hardware limit lies. No current increase or new actuation has been performed in response.

The current telemetry derives phase-current estimates from assumed 0.01 ohm shunts and gain 50; gain under PWM has not been externally calibrated. The controller is voltage torque mode with current-dependent foldback, not closed-loop q-axis current regulation. Doubling a cutoff does not command twice the torque-producing current. At equal resistance, doubling actual phase RMS current quadruples copper loss; ideal torque approximately doubles only if the torque-producing current doubles and magnetic/voltage limits do not intervene.

The manufacturer listing provides a 12 V / 1 A load point, 600–1000 g cm torque, 5.6 ohm unspecified internal resistance and maximum power <=25 W. It does not establish a 2.7 A continuous or pulse phase-current rating, nor a permissible pulse duration. The supplied resistance cannot be unambiguously treated as per-phase resistance. Cold housing after short trials is not a winding-temperature measurement.

Code review identifies an important difference from the historical sketch: the current total-command slew limit is 6 V/s, including command reversals and damping. Without intervening foldback/hard-current cuts, +1.5 to -1.5 V requires 0.5 s; +5.5 to -5.5 V requires 1.83 s. During part of a reversal the old command sign persists. This is a plausible contributor to indirect feel and energy injection, not yet a verified sole cause. The historical code updates its command on incoming packets with angle smoothing but has no equivalent voltage slew limiter. Earlier legacy trials reproduced its static spring law, not its full dynamics.

Next meaningful maximum-force experiment: an isolated single-motor, short fixed-direction torque test with measured lever force, verified current scaling and temperature monitoring, avoiding the bilateral oscillatory loop. Exact current steps and duration require those measurements; do not treat 2.7 A as a validated target. Both motors remain stopped while preparing the measurement.


## Direct-response correction

The user has a thermometer and multimeter but no scale, and asks to proceed toward a wearable prototype today without force measurement. Next trial retains the small-angle 10 V/rad gain, 5.5 V ceiling, 1.10/1.35 A current guards and zero synthetic damping. Change only the force-profile output dynamics: remove the 6 V/s total-command slew during interaction, so release, reversal and protective foldback follow the current request without the stale torque tail. A 500 ms engagement multiplier after peer synchronization softens startup only. Other profiles retain their existing slew behavior. The 30-second session limit and coordinated stop remain. No current increase is included in this correction. Numerical torque remains unmeasured and wearable acceptance remains pending.


### Direct-response hardware result

Both uploads verified at 115200 baud. The 30-second trial completed with A timer stop and B peer stop; both motors confirmed disabled. A peak was 1.325 A, B peak 1.472 A. B recorded two hard-current cuts, demonstrating transient overshoot above the configured 1.35 A software cutoff. Thermal-estimator foldback occurred during this trial; it remains a current-based estimate, not a measured temperature. See `main-force-direct-summary.json`. User perception and temperature assessment pending. No current-limit increase has been made.


## Short 1.8 A threshold trial

User assessment: direct-response behavior is somewhat better and much more immediate, but force remains insufficient. Motors/controllers feel approximately cold; numerical temperatures were requested using the available thermometer. User authorizes a stronger controlled experiment and reiterates increasing perceived elastic tension with relative angle across approximately 0–180 degrees.

Prepare a force-only threshold increase: soft 1.10 to 1.50 A, hard 1.35 to 1.80 A. Shorten this profile's firmware session from 30 to 10 seconds. Keep the 5.5 V ceiling, 10 V/rad gain, immediate protected response, startup engagement ramp, no synthetic damping, original thermal estimator, link timeout and stop behavior. Other profiles retain their current thresholds and the legacy profile retains its 30-second session. This is an incremental short pulse experiment, not a 2.7 A test or a continuous rating. The software cutoff may be exceeded transiently, as prior logs demonstrate. Verify measured peaks and temperature after the test before any further increase.

The current spring reaches its voltage cap around 31.5 degrees. Strong small-angle response and continued growth to 180 degrees compete for the same finite torque range. After adequate peak force is demonstrated, redistribute the characteristic over the desired angular range; do not claim the current linear profile grows through the full 180 degrees.

Validation and temperature/force results pending.


### First 1.8 A trial: inconclusive

Both uploads verified at 115200 baud. The first brief armed window showed negligible relative movement and only 0.147/0.133 A peak readings; the increased threshold was not exercised. A manual stop and B peer stop were confirmed. Do not treat this as validation of higher force or the firmware timer. Await user-ready signal for a meaningful repeat; numerical temperature readings have not yet been supplied.


### Numerical surface-temperature baseline

The user reports attaching a UNI-T UT139C temperature probe to the underside of a component and reading 32 degrees C. The exact component/node is not yet identified. Record this as one measured surface temperature, not both motor/controller temperatures or winding temperature. Keep the probe in place for a comparable post-trial reading. A meaningful loaded 1.8 A trial is still pending.


## Restart handshake correction

After an explicit user-ready signal, the repeated 1.8 A attempt entered repeated ARMED/peer-stop transitions, continually resetting local zero and peak counters before useful torque could develop. Manual stop confirmed both disabled. No useful high-current test resulted. The accepting peer did not receive the initiator's startup grace interval, and delayed run/stop echoes could repeatedly restart it. Add the same 150 ms grace when accepting a peer start, plus 250 ms suppression of peer-triggered restart after disarm. Explicit local start remains available. The actual peer-handling and disarm code passes deterministic host cases for startup grace, subsequent peer stop, delayed run-echo suppression and a later restart. Full hardware behavior remains to be verified; no claim that this explains earlier mechanical oscillation.


### Startup grace alone was insufficient

The startfix build stopped the repeated restart storm but both nodes still peer-stopped before useful torque; measured peak counters remained about 0.104 A. The 1.8 A threshold was again not exercised. Next correction reduces ESP-NOW attempts from 500 Hz to 100 Hz and allows only one packet in flight until its callback (or an immediate API failure). The next transmission uses current state rather than building a queue of older states. Startup grace becomes 500 ms on both initiator and accepting peer; the 250 ms post-disarm peer-start suppression remains. Sender/callback host tests verify gating, newest state after completion and error recovery. Packet backlog is a hypothesis for the premature stop, not yet a proven cause. A missing send callback leaves transmission blocked and the peer link timeout stops operation; no unbounded queue is introduced. Hardware validation pending.


### Radio-gated 1.8 A trial: completed

Both uploads verified at 115200 baud. After the advance warning, the 10-second loaded trial completed: one arm event per node, A timer stop, B peer stop, and both disabled. Current peaks were A 1.616 A and B 1.668 A. No hard-current cuts or thermal-estimator foldback occurred during recorded armed samples; soft-current and speed foldback still occurred. See `main-force-1800-radio-summary.json`. This verifies one start/run/stop cycle after the radio and grace changes; repeated restart and wearable acceptance remain pending. The controller surface baseline was 32 C; post-trial temperature and subjective force assessment are pending. No measured torque or continuous current rating is inferred.


### Temperature follow-up

The user reports that the probed controller surface was already 34 C immediately before the completed radio-gated 1.8 A trial and remained 34 C afterward. Therefore no change was observed at that surface point during this trial; the earlier 32 C reading must not be used to attribute a 2 C rise to the trial. Controller identity is still unspecified, and no numerical motor/winding temperature is available. Subjective force assessment remains pending.


## Responsive baseline accepted; 2.0 A short follow-up

The user reports a substantial improvement in responsiveness and perceived connection after the radio-gated 1.8 A trial, while requesting additional force. Preserve that source snapshot and manifest as a rollback baseline (`MKS_Parallel_Mirror-force-1800-radio.ino.txt`, `main-force-1800-radio-manifest.json`). The combined timing/radio changes are associated with the perceived improvement; their individual contribution has not been isolated. This is acceptance of the interaction direction, not wearable or sustained-load acceptance.

Next short trial changes only force-profile current thresholds from soft/hard 1.50/1.80 A to 1.75/2.00 A. Keep the same direct response, radio behavior, 10 V/rad spring, 5.5 V ceiling, zero added damping, thermal estimator and 10-second session timeout. Controller surface was reported 34 C before and after the preceding trial. No winding-temperature or force measurement is available. Compilation and hardware follow-up pending.


### 2.0 A short-trial result

Both uploads verified at 115200 baud. Both nodes completed their 10-second timer limits and remained disabled afterward; one arm per node. Current peaks A 1.879 A, B 1.815 A. No hard-current cuts or thermal-estimator foldback in recorded armed samples. Soft current and speed foldback occurred. User perception and post-trial controller surface temperature are pending. See `main-force-2000-summary.json`; neither observed peaks nor a short trial establish a continuous rating.


## Mild attack smoothing at the accepted 2.0 A setting

The user reports very responsive behavior and noticeable extra force, with the probed controller briefly at 35 C and returning to 34 C. The user requests a little smoothing to reduce sudden response while retaining force/speed, and further force only if feasible and safe. Motor/winding temperature remains unmeasured.

Retain the tested 1.75/2.00 A current thresholds and 5.5 V ceiling. Add force-magnitude attack limiting at 80 V/s, equivalent to at most about 69 ms from zero to the full 5.5 V command when otherwise unconstrained. Lower magnitude requests, release and protective foldback take effect immediately. A sign reversal resets the attack origin to zero, so no old-sign torque tail is retained. Static spring gain and final requested magnitude are unchanged; no viscous damping is added. This is not a stability proof or a guarantee of safer wearable operation. Startup engagement, radio handling and the 10-second trial limit remain unchanged.

Host checks exercise immediate release/foldback, correct sign on reversal, 1 ms attack increments, full-scale settling, no request overshoot and hard-current priority. Compilation and physical comparison pending.


### Mild attack smoothing trial result

Both uploads verified at 115200 baud. One arm per node; both 10-second timer stops observed, both disabled. Current peaks A 0.662 A / B 1.277 A. No current or thermal-estimator foldback in recorded armed samples. The lower observed load is not directly comparable with previous hand-moved trials and does not establish equal perceived peak force or its loss. User assessment of smoothing, force, responsiveness and controller temperature is pending. See `main-force-2000-soft-summary.json`.


## User-requested 30-second smoothing repeat

The user says the 10-second window is too short and explicitly requests a 30-second repeat. Change only the force-profile session timeout from 10 to 30 seconds. Retain 1.75/2.00 A current thresholds, 5.5 V ceiling, 10 V/rad spring, 80 V/s magnitude attack, immediate release, radio handling, current and thermal protections. The prior brief trial remains unassessed for feel; do not infer a verdict on smoothing from its low peak currents. Hardware repeat pending.


### 30-second smoothing repeat completed

Both uploads verified at 115200 baud. Full trial completed with one arm per node, A timer stop and B peer stop; both disabled. Peaks A 1.870 A / B 1.820 A; no hard-current cuts. Thermal-estimator foldback became substantial during the longer load, so any perceived reduction must be distinguished from attack smoothing. This estimator is not a physical temperature measurement. User assessment and controller surface temperature pending. See `main-force-2000-soft-30s-summary.json`.


## Requested 60-degree, two-minute trial

The user requests maximum spring demand at approximately 60 degrees, less early thermal foldback and a two-minute supervised test while monitoring motor temperature. The probe is now on a motor surface and reads 25 C before the trial; exact node unspecified.

Set the linear spring gain to 5.252113 V/rad, requesting 2.75 V at 30 degrees and 5.5 V at 60 degrees. This is an error-to-demand saturation point, not a physical joint travel limit. Small-angle demand is lower than the prior 10 V/rad profile. Retain 1.75/2.00 A instantaneous current thresholds, 5.5 V cap, 80 V/s magnitude attack, immediate release, startup engagement, radio and stop handling. Extend force session timeout to 120 seconds. Force-only thermal-estimate thresholds become 0.75/0.95 A instead of 0.45/0.60 A; the estimator time constant stays 45 seconds, and other profiles retain their earlier thresholds.

A 45 C motor surface reading is the chosen operator stop threshold for this trial, not a manufacturer-derived winding limit or a validated continuous rating. The external probe is not connected to firmware; the user must report readings and any fast temperature rise. Agent observes telemetry in short intervals and stops on user request or a concerning result. No torque measurement is available. Monitor readings apply to one motor surface, not both motors or windings.

Host tests verify the 30/60-degree spring law, force foldback at 0.70/0.85/0.95 A, unchanged legacy thermal thresholds, attack/release behavior and hard-current priority. Compilation passed; hardware trial pending.


### Two-minute trial completed

Both boards were uploaded and verified at 115200 baud. A stopped on the supervised session timer; B stopped on peer stop. Both report armed=0, auto=0 and zero commanded voltage. User-reported motor surface readings rose from 25 C baseline through 26, 27 and 28 C during the trial. Exact reading times and final post-stop temperature are not established; the probe identifies neither both motors nor winding temperature.

Node A: peak reported phase current 1.967 A, peak absolute Uq 5.500 V, minimum thermal scale 0.54, maximum thermal current estimate 0.841 A, hard-current cuts 0, speed-cut events 32.

Node B: peak reported phase current 1.833 A, peak absolute Uq 5.500 V, minimum thermal scale 0.58, maximum thermal current estimate 0.834 A, hard-current cuts 0, speed-cut events 18.

Thermal-estimator foldback remained active despite the modest measured case-temperature rise. Speed cuts also occurred; telemetry alone does not distinguish deliberate fast hand motion from oscillation. Operator assessment of sustained force, the 60-degree spring curve and smoothing remains pending. This test does not establish a continuous or wearable operating rating. See main-force-60deg-120s-summary.json and the corresponding raw logs.


### Operator assessment and revised small-angle curve

After completion, the user reports a maximum probed motor surface temperature of 30 C (25 C baseline), and again felt a substantial output reduction. The user requests 85–90% demand around 20–25 degrees, retaining gradual growth above that point. This is separate from the temperature reading: degrees below refer to angular error.

Implement a symmetric two-segment force curve: zero through the 0.01 rad deadband, continuous linear rise to 85% of the 5.5 V demand ceiling at 25 degrees, then a gentler linear rise to 100% at 60 degrees, saturating above 60 degrees. Approximate requests before all protection/startup scaling: 10 degrees 1.804 V, 20 degrees 3.718 V, 25 degrees 4.675 V, 30 degrees 4.793 V, 45 degrees 5.146 V, 60 degrees 5.500 V. These percentages are voltage-command fractions, not measured torque, current, mechanical power or percentages of hardware capability. The slope changes at 25 degrees; the command remains continuous. Existing magnitude-attack smoothing and immediate release remain.

Retain 1.75/2.00 A instantaneous limits, 0.75/0.95 A thermal-estimate foldback and the 120-second supervised timeout. The prior trial reached 5.5 V on both boards; boot supply estimates of 11.57/11.37 V imply only about 3–5% voltage headroom under the current centered SinePWM scheme, not a path to doubled torque. A modest external case-temperature rise does not calibrate the winding model or establish higher continuous current capability. No further peak or thermal limit increase is made in this revision. The stronger small-angle curve may reach foldback sooner under sustained load.

Host checks cover knee and saturation values, continuity at the deadband and knee, symmetry, monotonicity, existing attack/release and protection behavior. Physical assessment pending.

Both knee25 uploads completed at 115200 baud with flash hash verification. Serial checks confirm FOC ready and fresh mutual peer packets on both boards, force profile selected, armed=0, auto=0 and zero commanded voltage. No powered trial of this curve has been started.


### Knee25 two-minute trial completed

Started on explicit user authorization. Both nodes armed once and ran the supervised two-minute trial; both report timer stop, armed=0, auto=0 and zero Uq afterward. User reported 28 C then 30 C on the motor probe; a fresh pre-start baseline and final post-stop value were not reported.

Node A: peak reported phase current 1.878 A, maximum absolute Uq 5.319 V, minimum thermal scale 0.53, maximum thermal estimate 0.844 A, hard-current cuts 0, speed-cut events 65.

Node B: peak reported phase current 1.815 A, maximum absolute Uq 5.328 V, minimum thermal scale 0.56, maximum thermal estimate 0.838 A, hard-current cuts 0, speed-cut events 67.

Thermal-estimator foldback still reduced output. Speed-cut counts are events, not complete oscillation cycles; deliberate hand motion versus self-excited oscillation remains unconfirmed by the operator. Assessment of the new small-angle curve and final motor surface temperature is pending. See main-force-knee25-summary.json and raw logs.


## Requested 10-degree knee, small-ripple filter and wider thermal envelope

The user reports favorable progress with knee25 and requests approximately 68% at 10 degrees, gradual growth above that point, reduced thermal-estimator intervention, and suppression of small rapid oscillations. No separate final post-stop temperature was supplied.

Set force demand to zero inside 0.01 rad deadband, then linearly increase to 68% at 10 degrees and 100% at 60 degrees. Nominal settled demand fractions at 10/20/25/30/45/60 degrees are 68/74.4/77.6/80.8/90.4/100%. These are fractions of the 5.5 V command cap, not percentages of measured torque or power.

Add bounded error smoothing only for force: 20 ms low-pass time constant, with deviation from instantaneous error capped at 0.35 degrees outside the deadband. The output never has the opposite sign to raw error, never exceeds its magnitude and becomes zero immediately inside the deadband. Large changes therefore bypass most of the lag, and error reductions are immediate when the filtered value would otherwise exceed raw magnitude. State resets while disarmed or waiting for synchronization. Encoder/FOC input, transmitted position, speed protection, current protection and stop paths remain unfiltered. Status adds forceError in radians.

A synthetic host test with +/-0.15-degree ripple around a 5-degree offset retains 99.6% peak-to-peak at 1 Hz and 60.2% at 20 Hz with 1 ms updates. These results describe the signal filter only; they do not prove physical oscillation suppression or closed-loop stability. Filtering introduces bounded lag and asymmetric behavior during release. Current 2 Hz status logging cannot identify the actual oscillation spectrum; speed-cut counts are not frequency measurements. Human intent is not classified by this filter.

For the next supervised bench trial only, increase force thermal-estimate soft/hard thresholds from 0.75/0.95 A to 0.90/1.10 A, retaining the 45-second EWMA time constant. This is a bounded increase, not removal of protection or a calibrated temperature model. At a fixed 0.84 A estimate the new thermal scale is 1.0, but stronger output can drive the estimate higher and trigger foldback again. Retain 1.75/2.00 A instantaneous thresholds, 5.5 V command cap, 80 V/s attack and 120-second timer. The external motor surface probe and previous 45 C operator stop threshold remain required for the supervised trial. Current sensing and boot voltage scaling still lack independent gain verification; winding temperature and continuous capability are unknown.

### Remaining drive headroom

The last boot estimates were A 11.50 V and B 11.29 V. Centered SinePWM permits approximately half the bus voltage as Uq; compared with the 5.5 V cap, that implies about 4.5% and 2.6% nominal headroom respectively, before supply sag and measurement error. Raising a current threshold alone does not create more voltage or guarantee more torque. No instantaneous current/voltage increase is made in this revision.

The installed SimpleFOC 2.4.0 implementation supports midpoint-centered SpaceVectorPWM. Its theoretical linear voltage utilization is bus/sqrt(3), versus bus/2 for centered SinePWM (about 15.5% improvement). That is a possible separate supervised experiment, subject to current, sensing and thermal constraints, not an automatic torque gain. Switching modulation, closed-loop q-current commissioning or changing motor/gearing is outside this revision. The present tests do not establish this setup's absolute maximum torque. Sources: https://docs.simplefoc.com/pwm_modulation and https://docs.simplefoc.com/torque_loop . Manufacturer product information: https://shop.iflight.com/ipower-motor-gm3506-brushless-gimbal-motor-w-as5048a-encoder-pro1155 .

Host checks passed for curve values, continuity, symmetry, monotonicity, filter response, bounded lag, release, current priority, thermal scales and peer/radio handling; compilation passed. Powered assessment pending.

Both knee10-filter uploads completed at 115200 baud with flash hash verification. Both nodes report FOC ready, force profile, fresh mutual packets, armed=0, auto=0 and zero Uq/filter state. New boot voltage estimates are A 11.34 V and B 11.22 V (not live or meter-verified). Powered testing has not started.


### Knee10-filter two-minute session completed

Operator supplied a 26 C motor-surface baseline. Both nodes armed once; A stopped on the 120-second timer and B on peer stop. Both report armed=0, auto=0 and zero Uq/filter state. The initial roughly 25 seconds showed little motion or demand, so this was not 120 seconds of sustained loading. Operator reported 26 C and later 27 C during the session; final temperature is pending.

Node A: peak reported phase current 1.908 A, maximum absolute Uq 5.500 V, minimum thermal scale 0.80, maximum thermal estimate 0.940 A, hard-current cuts 0, speed-cut events 98.

Node B: peak reported phase current 1.844 A, maximum absolute Uq 5.500 V, minimum thermal scale 0.80, maximum thermal estimate 0.941 A, hard-current cuts 0, speed-cut events 86.

Thermal foldback was less severe in this session, but hand loading and active load duration differed, so this is not an isolated quantitative comparison. Speed events persisted and their origin remains unconfirmed; the filter is not physically accepted as suppressing self-excited oscillation. At the end of the session A reported about 6.53 rad local position while disabled; whether manually rotated or a sensing discontinuity is not established. Raw logs retained. Operator assessment of the 10-degree curve, sustained force, oscillation and final temperature is pending.


### Knee10-filter rejected; exact knee25 rollback

The user reports noticeably worse oscillation and a final probed motor surface temperature of 31 C (26 C baseline), and requests the previous version before the added filter. Restore the exact archived knee25 source, SHA-256 4a47e7478dc978d3bae9b560630391ff5444e22155804cb45ef33839a39a3bc5: 85% at 25 degrees, 100% at 60 degrees, no error filter, original 80 V/s magnitude attack, 0.75/0.95 A thermal-estimate envelope, 1.75/2.00 A instantaneous thresholds, 5.5 V cap, centered SinePWM and 120-second explicit-start sessions. This rolls back all three simultaneous experimental changes; it does not isolate which change caused the poorer behavior. In particular, a higher small-angle stiffness, reduced foldback and added filter lag may all contribute. Synthetic filter tests did not predict physical stability and must not be represented as physical validation.

The user asks whether SpaceVectorPWM is worth investigating. It remains a separate reversible candidate: first compare at unchanged Uq/current limits, then assess any use of increased voltage headroom only after acceptable behavior. Same capped Uq should not itself produce a large static torque gain; theoretical bus utilization is about 15.5% higher than SinePWM, not doubled torque and not an oscillation remedy. No PWM change is included in this rollback.


### Rollback scope corrected by the user: remove only the error filter

The user stopped work before any knee25 rollback upload. Both boards received manual stop and confirmed armed=0, auto=0, stopLatch=1 and zero Uq. The knee25 rollback existed only in local source/build, never on hardware.

The user then clarified that the latest 10-degree response and all other settings must remain; only the oscillation/error filter should be removed. Reconstruct from the archived knee10-filter source and remove only filterForceError, its state/reset/use and its status field. The spring uses instantaneous error again. Retain 68% at 10 degrees and 100% at 60 degrees, 0.90/1.10 A thermal-estimate thresholds, 1.75/2.00 A instantaneous limits, 5.5 V cap, 80 V/s magnitude attack, immediate release, centered SinePWM and the 120-second explicit-start timeout. No modulation change. This supersedes the full knee25 rollback plan above. The 31 C final reading and rejected filtered-trial assessment remain historical results.

Host curve and protection checks passed. Physical comparison of filter removal pending; no claim that the filter alone caused the previous oscillation increase.

Both knee10-no-filter uploads completed at 115200 baud with hash verification. Both report FOC ready, force profile, fresh peer packets, armed=0, auto=0 and zero Uq. Boot voltage estimates A 11.34 V / B 11.17 V. No powered trial started.


### Final knee10-no-filter session for the day

User requested one closing trial and no further tuning today if acceptable. Motor surface readings: 28 C baseline, 30 C during the first half, 34 C near completion, 35 C final. Both nodes armed once and stopped on their 120-second timers; both disabled with automatic start off.

Node A: peak reported current 1.843 A, maximum absolute Uq 5.500 V, minimum thermal scale 0.62, maximum thermal estimate 0.976 A, hard-current cuts 0, speed-cut events 169.

Node B: peak reported current 1.832 A, maximum absolute Uq 5.500 V, minimum thermal scale 0.65, maximum thermal estimate 0.970 A, hard-current cuts 0, speed-cut events 122.

Speed events increased sharply near the end; intentional hand movement versus self-oscillation remains unconfirmed. Operator acceptance is pending and must not be inferred from temperature alone. The exact source snapshot, manifest, raw logs, summary and flash binaries have been preserved. No additional tuning or PWM change is planned today.


### Operator clarification and end-of-day freeze

The operator confirms deliberately allowing oscillation near the end to observe its readings. This explains the test condition behind the increased speed-cut counts; it does not establish freedom from self-oscillation. The latest message gives 30 C as the final reading, after an earlier final report of 35 C and an in-trial report of 34 C. Use 30 C as the latest reported final value, retain the earlier values as reported history, and do not infer a verified maximum temperature. The summary records this discrepancy explicitly.

Stop work for the day on knee10-no-filter as the continuation baseline. Both boards received x and confirmed manual stop, armed=0, auto=0, stopLatch=1 and zero Uq. No further tuning, PWM switch or powered trial today. See end-of-day.md for the frozen configuration and resumption procedure.

Final temperature clarification: the user explicitly corrected the intervening 30 C report and confirmed **35 C** as the final motor-surface value. Use baseline 28 C / final 35 C in the closing record; the discrepancy is resolved. Both serial monitors were closed after verified manual stop.


## User-requested additional short SpaceVectorPWM comparison

After the day-end save, the user requested a quick test of the alternative PWM mode. The archived knee10-no-filter SinePWM baseline and binary remain intact. Prepare a 30-second explicit-start SpaceVectorPWM trial with centered modulation. Retain the 5.5 V Uq cap, conservative half-bus runtime cap, 1.75/2.00 A instantaneous limits, 0.90/1.10 A thermal-estimate range, 10-degree curve, 80 V/s magnitude attack, no error filter and existing radio/stop handling. Add pwm=SVPWM to status. The only functional changes from the saved baseline are modulation selection and the shortened trial timer.

This first comparison tests behavior at the same commanded voltage; it does not test the additional voltage headroom or promise increased torque. A larger Uq limit is not enabled. Motor surface baseline and physical readiness requested; no powered trial started yet.


### Same-voltage SpaceVectorPWM trial completed

User confirmed unchanged secured bench setup and 31 C motor surface before the test. Both uploads were hash-verified at 115200 baud; both reported pwm=SVPWM and FOC ready. One arm per node, 30-second A timer stop and B peer stop, both disabled.

Node A: peak reported current 1.870 A, maximum absolute Uq 5.500 V, minimum thermal scale 1.00, maximum thermal estimate 0.896 A, hard-current cuts 0, speed-cut events 5.

Node B: peak reported current 1.840 A, maximum absolute Uq 5.500 V, minimum thermal scale 1.00, maximum thermal estimate 0.862 A, hard-current cuts 0, speed-cut events 0.

No torque or PWM-synchronized current accuracy comparison was measured. Shorter duration, different hand movement and the request not to deliberately excite oscillation prevent direct comparison of event counts with the two-minute SinePWM trial. The additional SVPWM voltage headroom remains unused. Operator comparison and final temperature pending. Temporary SVPWM trial firmware currently remains installed, disabled; the archived SinePWM baseline remains available.


### SVPWM operator assessment and baseline restoration

Motor surface final 32 C, from 31 C before the test. User reports approximately unchanged feel, without a clear difference in warmth, oscillation or vibration. Slight scratchy/growling sensation at larger load remains, with no clear increase or decrease. This was a same-Uq-cap comparison; extra voltage headroom was not exercised, so the result does not reject potential headroom gains. Do not infer torque equivalence from uninstrumented feel or claim that SVPWM eliminated oscillation.

Restore the saved knee10-no-filter SinePWM source and archived flash files after this temporary comparison. No additional force or voltage experiment today. Archive checksums verified before upload; restoration hardware verification pending.


### Restoration interrupted: hardware reconnect required

Both SinePWM restoration uploads lost their USB ports during application flashing (around 44%). Both upload processes exited with serial port not found. Source and archived binaries are intact, but neither installed application is verified and both may be incomplete. The failed restore must be completed before use; do not describe the boards as restored or ready. User requested to reconnect both USB devices with motor batteries disconnected for recovery. No baud-rate escalation was attempted; uploads used 115200 throughout.


### Restoration recovered and verified

After the user reconnected USB, both archived knee10-no-filter SinePWM uploads completed at 115200 baud with flash hash verification. Original archived file checksums remain unchanged. Both boards booted, FOC ready, force profile selected, fresh mutual packets. Both received x and report armed=0, auto=0, stopLatch=1, lastStop=manual stop and Uq=0. Final recovery logs: restored-sine-1410.log / restored-sine-1430.log. Both boot VIN estimates were about 1.06 V on USB-only power; low-battery inhibition is expected. Before resuming powered operation, connect the motor supply and restart to refresh boot voltage. No motor actuation was requested after recovery. Serial monitors closed for end of day.
