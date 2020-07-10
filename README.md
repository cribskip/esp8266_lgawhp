# esp8266_lgawhp - MQTT bridge for LG AWHP
Bridge for 2018+ R32 LG Air/Water-Heat pumps. See https://www.lg.com/de/business/monobloc for more information on the unit.

# About

# Hardware

# Usage

# Outlook
1. Simulate PENKTH000 to leverage power usage control

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
00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19
--- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ---
A0 | 22 | 02 | 08 | 00 | 19 | 00 | 00 | 14 | 2D | 00 | 17 | 11 | 26 | C0 | 00 | 06 | 40 | 00 | 2F
SRC | MODE1 | MODE2 | MODE3 | MODE4 | **UNK** | **UNK** | **UNK** | Water_target | DHW_target | **UNK** | Water_In | Water_Out | DHW | **UNK** | **UNK** | **UNK** | **UNK** | **UNK** | ChSum
 
