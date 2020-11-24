void D0_Init() {
#ifndef DBG
  // AC communication, 300 baud 8N1. TX GPIO15, RX GPIO13
  Serial.begin(300);
  //Serial.swap();
  Serial.setRxBufferSize(64);
#endif
  lastCharTime = millis();
  state.sent = true;
}

inline void handle_D0() {
  D0_Check_RX();

  // Queue is full?
  if (Q_in.size() == 20) {
    D0_handle_recv_packet();
  }

  // Send to AWHP if data pending
  // &&   digitalRead(13) == LOW
  if (state.sent == false && (millis() - lastCharTime) > 200 && Q_out[19] != 0x00) {
    D0_send_packet();
  }
}

inline void D0_Check_RX() {
  // Read from LG AWHP
  if (Serial.available()) {
    uint8_t x = Serial.read();

    // new Packet has at least 125ms pause
    if (Q_in.isEmpty()  && (lastCharTime - millis() > 125)) {
      // First Byte: A0, A5, A6 or C0, C5, C6
      // Expect first byte to be A0 - FF, easy fix for sync to packets.
      // !! TODO / BUGFIX: This is not sufficient to be in sync with packets!!!
      uint8_t y = x & 0xF0;
      if (y == 0xA0 || y == 0xC0) {
        //y = x & 0x0F;
        //if (y == 0xA0 || y == 0xC0) {
        Q_in.push(x);
      }

    } else if (Q_in.size() > 0) Q_in.push(x);

    lastCharTime = millis();
  }
}

inline void D0_handle_recv_packet() {
  unsigned long sum;
  for (uint8_t i = 0; i < 20; i++) sum += Q_in[i];
  if (! isCheckSum(sum, Q_in[19])) {
    Q_in.clear();
    lastCRC = 0x00;
    return;
  }

  if (lastCRC != Q_in[19]) {
    // 20 chars received, remember CRC
    lastCRC = Q_in[19];
    Q_in.clear();
  } else {
    // Received two messages, parse
    pubMsg(Q_in); // Publish to MQTT
    lastMsgTime = millis();

    // A0 or C0
    if ((Q_in[0] & 0x0F) == 0x00) {
      uint8_t x;
      if (state.sent) {
        String mode;
        x = Q_in[1];
        if (x & 0x02) {
          // AWHP ON
          if (x & 0x08) mode = "AI";
          else if (x & 0x10) mode = "Heat";
          else if (x & 0x20) mode = "Cool";
        } else {
          // AWHP OFF
          mode = "OFF";
        }
        PUB("AWHP/mode", mode.c_str());
        PUB("AWHP/Silent",  Q_in[3] & 0x08 ? ON : OFF);
        PUB("AWHP/DHW/state", Q_in[3] & 0x80 ? ON : OFF);
      }

      //mqtt.publish("AWHP/State5", String(Q_in[5]).c_str());

      PUB("AWHP/HKP", Q_in[2] & 0x02 ? ON : OFF );
      PUB("AWHP/DHW_VALVE", Q_in[2] & 0x20 ? ON : OFF );
      PUB("AWHP/ODU", Q_in[3] & 0x02 ? ON : OFF );

      PUB("AWHP/In", String(Q_in[11]).c_str());
      PUB("AWHP/Out", String(Q_in[12]).c_str());

      if (Q_in[0] == 0xA0) {
        if (state.sent) {
          x = Q_in[8];
          if (x > 20 && x < 40) PUB("AWHP/target", String(x).c_str());
          PUB("AWHP/DHW/target", String(Q_in[9]).c_str());
        }

        float temp = 10;
        temp += Q_in[5] * 0.5;
        if (temp > 0 && temp < 35) PUB("AWHP/Indoor", String(temp).c_str());

        PUB("AWHP/DHW", String(Q_in[13]).c_str());

        // new valid Message, copy to out
        for (uint8_t i = 0; i < 20; i++) {
          Q_out[i] = Q_in[i];
        }
      }
    }

    // Clear buffer
    Q_in.clear();
    lastCRC = 0x00;
  }
}

inline void D0_send_packet() {
  uint8_t x;
  mqtt.publish("AWHP/debug", String(F("SEND")).c_str());
  _yield();

  pubMsg(Q_ovr);

  // Replace data from MQTT
  {
    uint8_t i = 0;
    for (i = 1; i < 19; i++) {
      x = Q_ovr[i];
      if (x != 0x00) {
        Q_out[i] = x;
        Q_ovr[i] = 0x00;
      }
    }
  }

  // Set bit for sender ID & 'new data indicator'
  Q_out[0] = 0x20;
  bitSet(Q_out[1], 0);

  // Calc CRC for packet
  Q_out[19] = calcCRC();
  pubMsg(Q_out);
  _yield();

  // Push out to AWHP
  for (uint8_t i = 0; i < 20; i++) {
    Serial.write(Q_out[i]);
  }
  Serial.flush();
  _yield();

  state.sent = true;
  Q_out[19] = 0x00;
}
