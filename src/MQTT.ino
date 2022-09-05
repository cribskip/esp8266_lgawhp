// function called when a MQTT message arrived
void callback(char* p_topic, byte * p_payload, unsigned int p_length) {
  // concat the payload into a string
  String payload;
  for (uint8_t i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
  }
  String topic = String(p_topic);

  mqtt.publish("AWHP/debug", topic.c_str());
  _yield();

  // handle message topic
  if (topic.startsWith("AWHP/command/")) {
    if (topic.endsWith("reset")) ESP.restart();
    else {
      // Write value as payload directly to Output buffer /x
      uint8_t reg = topic.substring(13).toInt();
      uint8_t val = payload.toInt();

      Q_out[reg] = val;
      state.sent = false;
    }
  } else if (topic.startsWith("AWHP/set/")) {
    if (topic.endsWith("mode")) {
      uint8_t newmode = 0x00;

      if (payload.equals("OFF")) newmode = 0x30;
      else if (payload.equals("Cool")) newmode = 0x22;
      else if (payload.equals("Heat")) newmode = 0x32;
      else if (payload.equals("AI")) newmode = 0x2E;

      Q_out[1] = newmode;
      state.sent = false;
    } else if (topic.endsWith("DHW")) {
      uint8_t newmode = Q_out[3];

      if (payload.equals("OFF")) newmode = bitClear(newmode, 7);
      else if (payload.equals("ON")) newmode = bitSet(newmode, 7);

      Q_out[3] = newmode;
      state.sent = false;
    } 
     else if (topic.endsWith("Silent")) {
      uint8_t newmode = Q_out[3];

      if (payload.equals("OFF")) newmode = bitClear(newmode, 3);
      else if (payload.equals("ON")) newmode = bitSet(newmode, 3);

      Q_out[3] = newmode;
      state.sent = false;
    }

    lastCharTime = millis() + 100;
  }
}
