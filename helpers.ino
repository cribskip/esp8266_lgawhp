void _yield() {
  yield();
  if (state.sent) mqtt.loop();
  yield();
  httpServer.handleClient();
  yield();

#if PENKTH000
  yield();
  mb.task();
  yield();
#endif
}

void hardreset() {
  ESP.wdtDisable();
  while (1) {};
}

void pubMsg_Arr(uint8_t* arr) {
  String msg;

  for (uint8_t i = 0; i < 20; i++) {
    if (arr[i] < 0x10) msg += "0";
    msg += String(arr[i], HEX);
    msg += " ";
  }

  msg.toUpperCase();
  PUB("AWHP/msg", msg.c_str());
}

void pubMsg_Circ(CircularBuffer<uint8_t, 20> &arr) {
  String msg;

  for (uint8_t i = 0; i < 20; i++) {
    if (arr[i] < 0x10) msg += "0";
    msg += String(arr[i], HEX);
    msg += " ";
  }

  msg.toUpperCase();
  PUB("AWHP/msg", msg.c_str());
}

void printSerialHex(uint8_t c) {
  if ((int)c < 16) Serial.print("0");
  Serial.print(c, HEX);
  Serial.print(F(" "));
}

void clear_out() {
  for (uint8_t i = 0; i < 20; i++) Q_out[i] = 0x00;
}

inline bool isCheckSum(unsigned int sum, uint8_t x) {
  //Serial.print (" SUM: ");
  //Serial.println(sum ^ 0x55 & 0xFF, HEX);
  if (sum == 0 || x == 0) return false;
  else return (sum ^ 0x55 & 0xFF == x);
}

uint8_t calcCRC() {
  unsigned int _txsum = 0;

  for (uint8_t i = 0; i < 19; i++) _txsum += Q_out[i];
  return  _txsum ^ (0x55 & 0xFF);
}
