# Legacy Firmware

This directory preserves the first-generation bilateral mirror sketch for the
earlier `FOC_ESP32_V1.1` controller.

It is retained for research history only. Its GPIO mapping, MAC addresses,
startup behavior, and protection model do not match the current Makerbase MKS
ESP32 FOC Mega prototype.

Do not flash the legacy sketch to the current hardware. Use
[`MKS_Parallel_Mirror`](../MKS_Parallel_Mirror/MKS_Parallel_Mirror.ino) for the
current supervised prototype.
