extern void pubMsg_Arr(uint8_t* arr);
extern void pubMsg_Circ(CircularBuffer<uint8_t, 20> &arr);

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
  //Q_out[19] != 0x00 &&
  if (state.sent == false && (millis() - lastCharTime) > 100) {
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
        //PUB("AWHP/rcv", String(x, HEX).c_str());
      }

    } else if (Q_in.size() > 0) Q_in.push(x);
    lastCharTime = millis();
  }
}

inline void D0_handle_recv_packet() {
  unsigned long sum = 0;
  for (uint8_t i = 0; i < 20; i++) sum += Q_in[i];
  if (! isCheckSum(sum, Q_in[19])) {
    Q_in.clear();
    lastCRC = 0x00;
    //PUB("AWHP/debug", "Not CRC");
    return;
  }

  if (lastCRC != Q_in[19]) {
    // 20 chars received, remember CRC
    lastCRC = Q_in[19];
    Q_in.clear();
    //PUB("AWHP/debug", "Dup");
  } else {
    // Received two messages, parse
    PUB("AWHP/debug", "OK");
    pubMsg_Circ(Q_in); // Publish to MQTT
    lastMsgTime = millis();
    // C6 01 or C6 03
    if (Q_in[0] == 0xC6)
    {
      if (Q_in[1] == 0x01)
      {
        PUB("AWHP/flow", String(Q_in[17]).c_str());   // Waterflow flow/10 in l/min, if flow=5 then flow=0
      }
      if (Q_in[1] == 0x03)
      {
        PUB("AWHP/pressure", String(Q_in[14]).c_str()); // Waterpressure pressure/10 in bar
        PUB("AWHP/outside_temp", String(Q_in[16]).c_str());
      }
    }  
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
        if (state.sent) {
          for (uint8_t i = 0; i < 20; i++) {
            Q_out[i] = Q_in[i];
          }
        }
      }
    }

    // Clear buffer
    Q_in.clear();
    lastCRC = 0x00;
  }
}

inline void D0_send_packet() {
  u8 x;

  mqtt.publish("AWHP/debug", String(F("SEND")).c_str());
  
  for (u8 _i = 0; _i<2; _i++) {  
    yield();

    // Set bit for sender ID & 'new data indicator'
    Q_out[0] = 0x20;
    //bitSet(Q_out[1], 0);

    //pubMsg_Arr(Q_out);

    // Calc CRC for packet
    Q_out[19] = calcCRC();
    pubMsg_Arr(Q_out);
    yield();

    // Push out to AWHP
    for (uint8_t i = 0; i < 20; i++) {
      Serial.write(Q_out[i]);
    }
    Serial.flush();
    delay(130);
  }

  state.sent = true;
  lastCharTime = millis();
}
