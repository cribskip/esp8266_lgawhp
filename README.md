# esp8266_lgawhp - MQTT bridge for LG AWHP
Bridge for 2018+ R32 LG Air/Water-Heat pumps. See https://www.lg.com/de/business/monobloc for more information on the unit.

# About
Based on the results from https://github.com/hww3/LG_Aircon_MQTT_interface

# Hardware
Same as https://github.com/hww3/LG_Aircon_MQTT_interface

# Usage

# Outlook
1. Complete documentation / usage
2. Simulate PENKTH000 to leverage power usage control

# LG AWHP Protocol
## Glossary
Abbrev | Meaning
--- | ---
*AWHP* | Air/Water heat pump
*ODU* | Outdoor unit
*IDU* | Indor unit (monobloc includes this in the ODU), not to be confused with the REMO
*REMO* | Remote for control

## General
  - Single-wire half-duplex 300 Baud serial bus on the "Signal" line between ODU and REMO
  - Packets are 20 Bytes in size
  - Each (non-changing) packet is transmitted twice
  - Each packet has a **checksum** in the last byte

## Packet

### Status update / command
00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19
--- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ---
x0 | 22 | 02 | 08 | 00 | 19 | 00 | 00 | 14 | 2D | 00 | 17 | 11 | 26 | C0 | 00 | 06 | 40 | 00 | 2F
SRC | MODE1 | MODE2 | MODE3 | MODE4 | **UNK** | **UNK** | **UNK** | Water_target | DHW_target | **UNK** | Water_In | Water_Out | DHW | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | ChSum

### Unknown
00 | 01
--- | ---
x5 | 
x6 | 01
x6 | 02
x6 | 03

### Energy monitoring
00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19
--- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ---
C6 | 00 | 00 | 00 | 00 | 00 | 00 | 00 | 00 | 00 | 0B | B8 | 00 | 00 | 00 | 00 | D0 | 07 | 00 | 35
SRC | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** 

# LG PENKTH000 Modbus
## General
  - Connection without GND with Modbus wires A and B
  - Serial parameters: 9600 baud, 8 bits of data, No parity, 2 Stopp bits (9600 8N2)
  - Protocol is standard Modbus RTU

## Tools / References
  - https://npulse.net/en/online-modbus
  - https://www.modbustools.com/modbus_slave.html
  - https://www.mikrocontroller.net/topic/500056
  
## Registers

Register | Type / length | Content | Unit
--- | --- | --- | ---
30010 | uint8 / 1 | energy consumption | W |
40000 | uint32 / 2 | impulses per kWh, Connection 1 | imp/kWh |
40002 | uint32 / 2 | impulses per kWh, Connection 2 | imp/kWh |
40004 | uint32 / 2 | impulses per kWh, Connection 3 | imp/kWh |
40006 | uint32 / 2 | impulses per kWh, Connection 4 | imp/kWh |
