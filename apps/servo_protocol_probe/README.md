# Servo Protocol Probe

Passive LA104 protocol recognizer for a running CAN bus. It auto-scans 125/250/500/1000 kbit/s and assigns evidence scores for:

- Hitec CAN Custom register packets (`r`, `v`, `R`, `V`, `w`, `W`, `x`, `X`);
- DroneCAN/UAVCAN v0 extended identifiers and transport tail bytes (including NodeStatus);
- CANopen NMT, heartbeat and SDO signatures;
- common J1939 PGNs.

It runs bxCAN in silent mode: no transmit and no ACK. It therefore cannot determine the protocol of a completely silent servo. Use `143mdbcan.elf` only when this probe shows Hitec CAN Custom and the servo is configured for CAN 2.0A.

Use an external 3.3 V CAN transceiver: LA104 P3=CAN_RX, P4=CAN_TX. Never connect CANH/CANL directly to LA104 GPIO.
