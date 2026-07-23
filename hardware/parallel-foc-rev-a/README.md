# Parallel FOC Rev A

Custom single-motor ESP32 FOC controller derived from the validated MKS ESP32
FOC Mega prototype. This directory will contain the editable KiCad source and
the generated manufacturing package.

## Rev A objectives

- preserve the validated MKS Mega motor-control mapping:
  - PWM: GPIO32 / GPIO33 / GPIO25
  - driver enable: GPIO12
  - current sense: GPIO39 / GPIO36
  - AS5048A SPI: SCK 18 / MISO 19 / MOSI 23 / CS 5
- retain 12-24 V input, reverse-polarity protection, 3-phase inverter and
  two-phase INA240A2 inline current sensing
- reliable USB-C programming and serial diagnostics
- correctly wired addressable RGB status LED
- continuous battery/input-voltage measurement while Wi-Fi and ESP-NOW run
- accessible RESET, BOOT and USER buttons
- retain autonomous operation without USB power
- add explicit test points for power rails, UART, BOOT/RESET, PWM, enable,
  current sense and encoder signals

## Confirmed corrections from the source schematic

### USB-C

The published MKS Mega schematic does not show the two USB-C configuration
channel pull-downs. Rev A will use one 5.1 kOhm Rd resistor from CC1 to GND and
one 5.1 kOhm Rd resistor from CC2 to GND so a USB-C host identifies the board as
a power sink and enables VBUS. D+/D- from both connector orientations will be
joined correctly and protected with a low-capacitance USB ESD array.

This failure mode is now reproduced on the project hardware: USB-A to USB-C
works, while USB-C to USB-C does not enumerate. That behavior is consistent
with missing CC pull-downs and makes the CC correction a required Rev A change.

The CH340C USB-UART bridge will retain crossed UART routing (bridge TX to ESP32
RX0, bridge RX to ESP32 TX0). DTR and RTS will drive the standard ESP32
automatic download/reset circuit. Dedicated BOOT and RESET buttons provide a
manual recovery path.

### RGB LED

The existing boards have the upper VDD and DI pads of the
XL-3528RGBW-WS2812B footprint interchanged. Rev A will use a verified symbol and
footprint with VDD connected to 3.3 V, GND to GND, and DI driven from GPIO2
through a small series resistor. The placement will include an unambiguous pin-1
mark and a nearby test pad.

GPIO2 is an ESP32 strapping pin. Before schematic freeze, either the selected
LED's power-up input behavior must be verified not to disturb boot, or the LED
data signal will be moved to a non-strapping GPIO and the firmware mapping will
be updated.

### Battery/input-voltage monitor

The MKS board measures VIN on GPIO13, which belongs to ADC2 and cannot be used
reliably while Wi-Fi/ESP-NOW is active. Rev A will instead route the voltage
divider to GPIO35 (ADC1), allowing continuous monitoring during operation.

Initial divider values are 150 kOhm from protected VIN to the sense node and
22 kOhm from the sense node to GND. This produces approximately 1.61 V at a
fully charged 3S pack (12.6 V) and 3.07 V at the board's 24 V maximum. The
sense node will include a 100 nF filter capacitor, an optional series resistor,
ADC protection, and a labeled test point. Final resistor values and tolerances
will be checked against the selected ESP32 module, ADC calibration method, and
the confirmed maximum input voltage before release.

Firmware will average calibrated ADC1 readings and use the RGB LED for startup
and low-battery indication. Voltage-based percentage is only an estimate under
motor load; cell-level undervoltage protection remains the responsibility of a
protected pack/BMS. A true multi-cell fuel-gauge IC can be added later if
state-of-charge accuracy is required.

## Planned editable design files

- `parallel-foc-rev-a.kicad_pro` - KiCad project
- `parallel-foc-rev-a.kicad_sch` - hierarchical schematic
- `parallel-foc-rev-a.kicad_pcb` - PCB layout
- project-specific symbols and footprints where required
- STEP model and board 3D preview

## Planned manufacturing package

- Gerber X2 copper, solder-mask, silkscreen and board-outline layers
- Excellon plated/non-plated drill files and drill map
- IPC-356 netlist for independent connectivity checking
- BOM with manufacturer part numbers and approved alternates
- CPL/centroid position file for SMT assembly
- assembly drawings, fabrication notes and schematic PDF
- pick-and-place polarity/orientation review sheet
- electrical bring-up and acceptance-test checklist

## Items required before schematic freeze

- clear, perpendicular photographs of both PCB sides and readable IC markings
- measured mounting-hole coordinates, connector positions and enclosure limits
- preferred assembler (PCBWay or JLCPCB) and whether they should assemble the
  power MOSFETs and terminal blocks
- required continuous/peak phase current and intended battery/input range

## Source reference

Makerbase publishes `Hardware/MKS ESP32 FOC Mega_SCH.pdf` in the
`makerbase-motor/MKS-ESP32FOC` GitHub repository. The PDF is sufficient as a
reference schematic, but it does not contain editable PCB layout or fabrication
data; Rev A therefore requires a new, independently checked layout.
