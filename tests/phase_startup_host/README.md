# Phase startup control-flow checks

The test compiles the actual phase startup sketch against minimal host IO stubs.
It checks no automatic start/repeat, the complete six-vector sequence, buffered
stop priority, recalibration before motion, refused supply conditions, and
current/encoder/supply/travel/time-limit stops with persistent fault latching.
It does not emulate motor physics, ESP32 PWM or ADC, serial timing, or wiring.

Run from the repository root with a host C++17 compiler:

```sh
clang++ -std=c++17 -Wall -Wextra -I tests/phase_startup_host tests/phase_startup_host/test.cpp -o /tmp/parallel-phase-startup-host-test
/tmp/parallel-phase-startup-host-test
```

On the current Mac, default C++ header lookup failed. The successful command
additionally supplied
`-isystem /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1`.
The separate Arduino build validates compatibility with the real library.
