LA104 Tests menu

Copy this entire tests directory to the root of the LA104 SD card.

Included applications:
 - I2C Sensor Test: scan and inspect I2C sensors.
 - CAN Monitor: passive CAN traffic monitor with auto-baud.
 - MDB950 Doctor: read-only Hitec MDB950 diagnostic tool.
 - Servo Protocol Probe: passive recognition of Hitec, DroneCAN,
   CANopen and J1939 traffic. The bottom status bar shows CAN state,
   baud rate, recognized protocol and ESR.

Passive tools never transmit or acknowledge CAN frames. A silent servo
cannot be identified until it emits traffic or receives a valid request
from an appropriate controller.