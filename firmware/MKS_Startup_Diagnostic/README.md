# Replacement-pair startup diagnostic

Stage 1 for MKS ESP32 FOC Mega + GM3506/AS5048A. This standalone sketch
holds GPIO12 enable and all three phase PWM pins low. It has no motor actuation,
radio connection, stored calibration, or encoder OTP writes. Disconnect external
motor power before flashing; GPIO states during reset are not controlled by the
sketch. Verify connector pin order before powering a newly wired encoder.

Uses the existing SPI mapping: SCK 18, MISO 19, MOSI 23, CS 5, 1 MHz, MODE1.
Reports the station MAC using `esp_read_mac`, without starting Wi-Fi.

At 115200 baud, records encoder angle, signed cumulative travel, diagnostics,
magnitude, parity and device error counters. Samples approximately 100 times
per second and prints twice per second. `r` resets counters and travel; `?`
prints status. Neither command can enable a motor.

An `encoder=OK` sample requires valid parity, no SPI error flags, completed
offset compensation, and no CORDIC/comparator warning. This is a sample-level
check, not a complete hardware acceptance result. All-zero disconnected SPI
responses fail the offset-compensation check. Angle/travel retain the last
healthy sample during errors; travel does not bridge invalid samples.

After startup, reset counters, then check stationary readings and slowly rotate
the shaft in both directions. If mechanically possible, verify roughly 360
degrees of travel over one complete turn. Check the encoder cable gently for
intermittent contact without unplugging it under power. Record counters before
and after each trial; require no new invalid frames or unhealthy samples.

Continue separately with current-sense offsets, controlled motor phase tests,
fresh FOC calibration for each new assembly, and the bilateral radio test.
Do not load the old pair's autonomous firmware with its old MAC/calibration.

Prepared 2026-09-14 using Arduino CLI 1.4.1 and ESP32 core 3.3.11.
Hardware acceptance is pending; compilation alone does not validate wiring.
