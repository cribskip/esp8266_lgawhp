#define WIFI_SSID "YOURWIFISSID"
#define WIFI_PASSWORD "YOURWIFIPASS"
#define BROKER "IPOFMQTTBROKER"

#define ON String("ON").c_str()
#define OFF String("OFF").c_str()

#define PUB(x,y) mqtt.publish(x, y)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>   // Local WebServer used to serve the configuration portal
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoOTA.h>
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

#include <PubSubClient.h>       // MQTT client
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

#include <CircularBuffer.h>
CircularBuffer<uint8_t, 20> Q_in;
uint8_t Q_out[20];
uint8_t Q_ovr[20];
uint8_t lastCRC = 0x00;

// Communication management
unsigned long lastCharTime = 0;
unsigned long lastMsgTime = 0;
struct {
  uint8_t sent: 1;
  uint8_t do_parse: 1;
} state;

void _yield() {
  yield();
  mqtt.loop();
}

void reconnect() {
  int oldstate = mqtt.state();

  // Loop until we're reconnected
  if (!mqtt.connected()) {
    // Attempt to connect
    if (mqtt.connect(String(String("AWHP") + String(millis())).c_str())) {
      mqtt.publish("AWHP/node", "ON", true);
      mqtt.publish("AWHP/debug", "RECONNECT");
      mqtt.publish("AWHP/debug", String(millis()).c_str());
      mqtt.publish("AWHP/debug", String(oldstate).c_str());

      // ... and resubscribe
      mqtt.subscribe("AWHP/command/#");
      mqtt.subscribe("AWHP/set/#");
    }
  }
}

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

      Q_ovr[reg] = val;
      state.sent = false;
    }
  } else if (topic.startsWith("AWHP/set/")) {
    if (topic.endsWith("mode")) {
      uint8_t newmode = 0x00;

      if (payload.equals("OFF")) newmode = 0x01;
      else if (payload.equals("Cool")) newmode = 0x22;
      else if (payload.equals("Heat")) newmode = 0x32;
      else if (payload.equals("AI")) newmode = 0x2E;

      if (newmode != 0x00) {
        Q_ovr[1] = newmode;
        state.sent = false;
      }
    } else if (topic.endsWith("DHW")) {
      uint8_t newmode = 0x00;

      if (payload.equals("OFF")) newmode = 0x20;
      else if (payload.equals("ON")) newmode = 0x80;

      if (newmode != 0x00) {
        Q_ovr[3] = newmode;
        state.sent = false;
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(2000);

  // AC communication, 300 baud 8N1. TX GPIO15, RX GPIO13
  Serial.begin(300);
  Serial.swap();
  Serial.setRxBufferSize(64);

  lastCharTime = millis();

  WiFi.setOutputPower(20.5); // this sets wifi to highest power
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }

  httpUpdater.setup(&httpServer, "admin", "");
  httpServer.begin();
  MDNS.begin("AWHP");
  MDNS.addService("http", "tcp", 80);

  mqtt.setServer(BROKER, 1883);
  mqtt.setCallback(callback);
  state.sent = true;
}

///////////////////////////////////////////////////////////////////////////////

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    mqtt.publish("AWHP/debug", String(F("NO WIFI")).c_str());
    _yield();
    ESP.restart();
  }
  if (!mqtt.connected()) reconnect();
  httpServer.handleClient();
  _yield();

  // Read from LG AWHP
  while (Serial.available()) {
    uint8_t x = Serial.read();
    lastCharTime = millis();

    // First Byte: A0, A5, A6 or C0, C5, C6
    if (Q_in[0] & 0xC0 || ((Q_in.size() == 0) && (x & 0xA0 ))) {
      Q_in.push(x);
    }
  }

  // Expect first byte to be A0 - FF, easy fix for sync to packets
  if (Q_in[0] < 0x9F) Q_in.clear();

  // Queue is full?
  if (Q_in.size() == 20) {
    state.do_parse = false;

    // Prevent double parsing
    if (lastCRC == Q_in[19]) Q_in.clear();
    else {
      // 20 chars received, check for validity
      lastCRC = Q_in[19];

      unsigned long sum;
      for (uint8_t i = 0; i < 20; i++) sum += Q_in[i];
      state.do_parse = isCheckSum(sum, Q_in[19]);
    }

    if (state.do_parse) {
      // new valid Message
      for (uint8_t i = 0; i < 20; i++) {
        uint8_t x = Q_in[i];
        if (Q_in[0] == 0xA0) Q_out[i] = x;
      }
      pubMsg(Q_in);
      lastMsgTime = millis();

      switch (Q_in[0]) {
        case 0xA0:
          if (state.sent == true) {
            //mqtt.publish("AWHP/State1", String(Q_in[1]).c_str());
            {
              String mode;
              uint8_t x = Q_in[1];
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
            }

            if (Q_in[2] & 0x20) PUB("AWHP/DHW/state", ON);
            else PUB("AWHP/DHW/state", OFF);

            //mqtt.publish("AWHP/State2", String(Q_in[2]).c_str());
            if (Q_in[2] & 0x02) PUB("AWHP/HKP", ON);
            else PUB("AWHP/HKP", OFF);

            //mqtt.publish("AWHP/State3", String(Q_in[3]).c_str());
            if (Q_in[3] & 0x02) PUB("AWHP/ODU", ON);
            else PUB("AWHP/ODU", OFF);

            if (Q_in[3] & 0x08) PUB("AWHP/Silent", ON);
            else PUB("AWHP/Silent", OFF);

            //if (Q_in[3] & 0x20) PUB("AWHP/DHW/state", ON);
            //else PUB("AWHP/DHW/state", OFF);

            //mqtt.publish("AWHP/State4", String(Q_in[4]).c_str());
            //mqtt.publish("AWHP/State5", String(Q_in[5]).c_str());

            mqtt.publish("AWHP/target", String(Q_in[8]).c_str());
            mqtt.publish("AWHP/DHW/target", String(Q_in[9]).c_str());
          }
          break;

        case 0xC0:
          mqtt.publish("AWHP/In", String(Q_in[11]).c_str());
          mqtt.publish("AWHP/Out", String(Q_in[12]).c_str());
          mqtt.publish("AWHP/DHW", String(Q_in[13]).c_str());
          break;
      }

      // Clear buffer
      Q_in.clear();
    }
  }

  // Send to AWHP if data pending
  if (digitalRead(13) == LOW && state.sent == false && Q_out[19] != 0x00 && (millis() - lastCharTime) > 400) {
    uint8_t x;
    mqtt.publish("AWHP/debug", String(F("SEND")).c_str());
    _yield();

    //pubMsg(Q_ovr);

    // Replace data from MQTT
    {
      uint8_t i = 0;
      for (i = 1; i < 10; i++) {
        x = Q_ovr[i];
        if (x != 0x00) {
          Q_out[i] = x;
          Q_ovr[i] = 0x00;
        }
      }
    }

    // Set bit for sender & 'new data'
    Q_out[0] = 0x20;
    bitWrite (Q_out[1], 0, 0);

    // Calc CRC for packet
    Q_out[19] = calcCRC();
    pubMsg(Q_out);
    _yield();

    for (uint8_t i = 0; i < 20; i++) {
      x = Q_out[i];
      Serial.write(x);
    }
    Serial.flush();
    _yield();

    bitWrite (Q_out[1], 0, 1);
    Q_out[19] = calcCRC();

    _yield();
    delay(250);
    for (uint8_t i = 0; i < 20; i++) {
      x = Q_out[i];
      Serial.write(x);
    }
    Serial.flush();
    _yield();

    state.sent = true;
    Q_out[19] = 0x00;
  }

  // Watchdog reset if there is no communication
  if ((millis() - lastCharTime) > 30000 || (millis() - lastMsgTime) > 30000) {
    wdt_enable(WDTO_15MS);
    Serial.println(F("RX TIMEOUT, WDT RESET!!"));
    mqtt.publish("AWHP/debug", String(F("WDT RESET")).c_str());
    _yield();

    while (true) {
      ;;;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
