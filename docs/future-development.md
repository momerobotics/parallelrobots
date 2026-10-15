# Future development directions

## Unified commissioning and persistent assembly calibration — deferred

Decision recorded 2026-09-14: retain the present manual, separate-sketch workflow
for new systems. The proposed simplification is a later development direction,
not part of the current demonstration work or an already available capability.
Use [manual commissioning](new-system-commissioning.md) for the current process.

Possible future design:

- One firmware offers an explicitly started first-assembly commissioning mode.
- Missing/invalid motor calibration leaves drive disabled rather than guessing.
- Validated motor electrical zero, direction, pole pairs and assembly identity
  are stored persistently, separately from board identity and VIN calibration.
- Records include a version and integrity check, with export/import for board
  replacement or recovery after complete flash erasure.
- Ordinary startup reuses accepted calibration and measures stationary current
  zero under the actual supply; it does not repeat the motor alignment sequence.
- Motor/encoder/phase-mapping changes require an explicit new commissioning action.
- Peer pairing and A/B role assignment could become an explicit user operation
  rather than source-code MAC edits.

Current limitations remain unchanged: main calibration is hardcoded by node;
NVS stores the tactile profile, not motor-calibration records; hardware swaps
are not reliably detected automatically; there is no automatic peer discovery.
This document does not authorize silently changing those behaviors during the
current hardware bring-up. Implementation, migration of existing records and
acceptance tests will be scoped separately.
