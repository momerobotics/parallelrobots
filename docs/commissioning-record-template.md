# Manual commissioning record — COPY FOR EACH PAIR

Date / operator / pair label:
Source revision or saved snapshot:
Arduino CLI / ESP32 core / Simple FOC versions:
Log directory / build artifact location:

## Assembly identity

| Field | A | B |
| --- | --- | --- |
| Physical board label / revision | PENDING | PENDING |
| Station MAC, printed byte order | PENDING | PENDING |
| USB port at test time | PENDING | PENDING |
| Physical motor/encoder label | PENDING | PENDING |
| Motor model / pole pairs | PENDING | PENDING |
| Phase terminal-to-wire map / connector orientation | PENDING | PENDING |
| Battery label | PENDING | PENDING |
| A–B / B–C / C–A winding resistance | PENDING | PENDING |

## Measurements — retain raw logs, not only this table

| Field | A | B |
| --- | --- | --- |
| Stationary encoder flags / counters | PENDING | PENDING |
| Manual rotation / full-turn trace | PENDING | PENDING |
| Cable-flex check | PENDING | PENDING |
| USB current zero A/B, mV | PENDING | PENDING |
| Battery-powered current zero A/B, mV | PENDING | PENDING |
| Firmware VIN / meter VIN at board terminals | PENDING | PENDING |
| Accepted VIN scale and residual error at tested levels | PENDING | PENDING |
| Phase-test revision / voltage | PENDING | PENDING |
| Phase endpoints / peak indicator / stop reason | PENDING | PENDING |
| Operator's noise/motion observation | PENDING | PENDING |
| Calibration revision / alignment voltage | PENDING | PENDING |
| Measured direction, CW or CCW | PENDING | PENDING |
| zeroE candidate 1 / 2 / further repeats, radians | PENDING | PENDING |
| Circular difference and approved tolerance | PENDING | PENDING |
| Forward/reverse pole estimates / return error | PENDING | PENDING |
| **Accepted zeroE, radians** | **PENDING** | **PENDING** |
| Independent FOC motion verification | PENDING | PENDING |
| Uploaded main binary hash / matching source | PENDING | PENDING |

## Pair acceptance

- [ ] Physical wiring/continuity checked with power disconnected.
- [ ] Both encoder and cable tests passed with preserved logs.
- [ ] Idle-current and VIN checks passed under the actual supply.
- [ ] Both phase tests passed, including operator observation.
- [ ] Calibration method itself validated; both motor calibrations accepted.
- [ ] Both MAC arrays updated in link and main sketches.
- [ ] Radio-only test passed in both directions.
- [ ] Controlled main build passed with automatic start initially disabled.
- [ ] Main stop / link-loss / latch / restart checks passed.
- [ ] Handoff startup and 10-minute cutoff checked.
- [ ] Two-battery, USB-disconnected test passed.
- [ ] Thermal and intended mounting/feedback checks recorded.
- [ ] Final source, binaries, mapping and logs archived.

Failure events, changes and repeats:

Outstanding work / limitations / final acceptance decision:

Never fill an unknown calibration with the old pair's number or a guessed zero.
