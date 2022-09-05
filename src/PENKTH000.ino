#if PENKTH000
void RS485_Init() {
  pinMode(RTS, OUTPUT);
  digitalWrite(RTS, LOW);

  RS485.begin(9600, SWSERIAL_8N1, TX, RX, false, 256);

  mb.begin(&RS485, RTS);

  mb.slave(SLAVE_ID);

  // FC01 0x Coil Status
  mb.addCoil(0, 0, 2);
  mb.Coil(0, 0);
  mb.Coil(1, 0);

  // FC02 1x Input Status
  mb.addIsts(0, 0, 1);
  mb.Ists(0, 1);

  // FC03, 4x Holding Registers / Impulses per kWh
  mb.addHreg(0, 0, 8);
  mb.Hreg(0, 10); // Power Meter 1 / L1
  mb.Hreg(2, 10); // Power Meter 2 / L2
  mb.Hreg(4, 10); // Power Meter 3 / L3
  mb.Hreg(6, 10); // Heat Meter

  // FC04, 3x Input Registers / Zählerstände
  mb.addIreg(0, 0, 16);
  mb.Ireg(8, 3500);
}
#endif
