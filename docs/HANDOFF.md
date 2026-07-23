# Parallel Robots — Supervised Operation Guide

This is a research prototype, not a finished product. Operate it only under
supervision.

The two motors exchange their angular positions wirelessly and create a
spring-like bilateral resistance.

## Before Startup

- Node A and node B each use a separate protected 3-cell LiPo battery.
- Use only charged, undamaged batteries.
- Keep the full mechanical travel area clear of fingers, hair, clothing, cables,
  and other obstructions.
- Do not restrain the motors or move the mechanisms during the first 8 seconds
  after startup.
- The onboard RGB LED does not work because of a PCB manufacturing error. An
  unlit LED does not indicate a fault.

## Startup

1. Place both mechanisms in comfortable starting positions with their travel
   paths unobstructed.
2. Switch on or connect the battery for node A.
3. Switch on or connect the battery for node B.
4. Do not touch either mechanism for 8 seconds.
5. Move one arm by only a few degrees. The other arm should follow gently and
   produce spring-like resistance.

If no resistance is present after 15 seconds, switch off both nodes. Do not try
to wake the system by moving or restraining the mechanisms.

## Operation

- A test session is limited to 10 minutes. The firmware then stops both motors
  automatically.
- Keep the recommended rotation within approximately 180–200 degrees of the
  starting position.
- Do not rotate the mechanisms through multiple complete turns or hold them at
  their travel limits.
- Do not restrain a motor for more than 2 seconds.
- Normal behavior is quiet, with a light spring-like or subtly stepped feel.
- Stop immediately if you notice strong vibration, knocking, a burning smell,
  a hot motor or controller, or a sudden forceful movement.

## Shutdown and Fault Recovery

1. Release the mechanisms.
2. Switch off or disconnect both batteries.
3. Wait at least 5 minutes before another test.

After a communication fault, the system enters a latched shutdown and does not
restart automatically. Power-cycle both nodes to restart it.

Do not restart after abnormal noise, vibration, or heating. Report the event to
the developer first.

## Important Restrictions

- Always disconnect a battery from its controller before charging it.
- Never connect the two batteries to each other.
- Use a USB-A-to-USB-C cable for servicing. This board revision does not power
  up reliably from a USB-C-to-USB-C cable.
- Never leave the prototype powered without supervision.

Firmware: `MKS_Parallel_Mirror`, supervised handoff mode, protocol version 7.
