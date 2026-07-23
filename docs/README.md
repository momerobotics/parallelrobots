# Documentation

This directory contains the operating, safety, development, and validation
documentation for the Parallel Mirror Steer-by-Wire research prototype.

## Start Here

- [Project overview and setup](../README.md)
- [Supervised operation guide](HANDOFF.md)
- [Reproduction guide](reproducibility.md)
- [Prototype readiness checklist](prototype-checklist.md)
- [Development log](development-log.md)

## Document Scope

| Document | Purpose |
| --- | --- |
| [`../README.md`](../README.md) | Research context, system architecture, hardware mapping, control strategy, setup, and limitations |
| [`HANDOFF.md`](HANDOFF.md) | Required startup, operation, shutdown, and fault-recovery procedure for supervised demonstrations |
| [`reproducibility.md`](reproducibility.md) | Exact hardware, software versions, wiring, calibration, build commands, staged bring-up, and acceptance checks |
| [`prototype-checklist.md`](prototype-checklist.md) | Validated functions and outstanding work before extended or repeatable user testing |
| [`development-log.md`](development-log.md) | Chronological engineering record and validation results |
| [`../firmware/README.md`](../firmware/README.md) | Firmware variants, build requirements, and safe bring-up order |
| [`../hardware/parallel-foc-rev-a/README.md`](../hardware/parallel-foc-rev-a/README.md) | Requirements and design notes for the planned custom controller |
| [`board-overview.png`](board-overview.png) | Product image of the Makerbase MKS ESP32 FOC Mega board used by the current prototype |

## Language

English is the primary language of the repository documentation.
[`HANDOFF-HU.md`](HANDOFF-HU.md) is retained as a Hungarian translation of the
supervised operation guide for local operators.

## Safety Status

The repository describes an experimental research platform. It is not a
clinically validated, production-ready, or unattended-use device. Follow the
[supervised operation guide](HANDOFF.md) and complete the relevant items in the
[prototype checklist](prototype-checklist.md) before testing.
