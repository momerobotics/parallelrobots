# FOC calibration control-flow checks

Runs the actual calibration sketch with the shared phase-test IO stubs. An ideal
rotor follows the generated phase field; tests cover CW/CCW conventions, encoder
wraparound, bounded phase voltages, no automatic start/repeat, a stationary rotor,
stop priority, current-zero recalibration, fault shutdown and fault latching.
This does not simulate friction, settling, electrical hardware or ADC accuracy.

```sh
clang++ -std=c++17 -Wall -Wextra -isystem /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1 -I tests/phase_startup_host tests/foc_startup_host/test.cpp -o /private/tmp/parallel-foc-startup-host-test
/private/tmp/parallel-foc-startup-host-test
```

Passed on 2026-09-14. Hardware repeatability and independent FOC validation remain
separate acceptance checks.
