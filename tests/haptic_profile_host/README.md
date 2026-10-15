# Main voltage-limit and spring regression checks

```sh
python3 tests/haptic_profile_host/check.py
```

The check compiles the actual firmware profile definitions and spring/runtime-limit
functions with minimal host stubs. It verifies restoration of motor/controller
voltage limits after the temporary 1 V initialization limit, immediate removal
of drive at a hard-current event, and finite, symmetric, bounded, monotonic spring
curves with the intended small-angle gain increase. It checks that the runtime
restoration call occurs after motor initialization in the main source.

This is not a closed-loop plant simulation or a stability/force certification.
Oscillation, current behavior, physical force and heating require hardware tests.
