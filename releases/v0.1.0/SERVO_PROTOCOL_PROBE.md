# Servo Protocol Probe v0.1.0

- File: 144servoprobe.elf
- Target: LA104 + gabonator/LA104 firmware
- Size: 6885 bytes
- SHA-256: 0acb84c1dda98c79fd5b65fb8303d0046a144b1c72803c3bd9cc53f32da9eb62
- Operation: STM32 bxCAN silent mode, no transmit, no CAN ACK.

The probe scans 125/250/500/1000 kbit/s. It scores Hitec Custom, DroneCAN/UAVCAN v0, CANopen and J1939 signatures; the result is a traffic classification, not a guarantee of a servo model. A completely silent device cannot be identified passively.