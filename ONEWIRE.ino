#if ONEWIRE
inline void handle_onewire() {
  if (millis() > next_OW) {
    if (bitRead(next_OW, 0) == 0) {
      sensors.requestTemperatures();  // async

      next_OW = millis() + 380; // Wait Xms for Y Bits solution(X/Y): (94/9), (188/10), (375/11)
      bitSet(next_OW, 0);
    } else {
      float temp = 0;
      for (uint8_t i = 0; i < 3; i++) {
        temp = sensors.getTempCByIndex(i);

        if (temp > -127)
          PUB(String(String(F("AWHP/sensors/")) + String(i)).c_str(),
              String(temp).c_str());
      }

      next_OW = millis() + 15000; // Wait 15s
      bitClear(next_OW, 0);
    }
  }
}
#endif
