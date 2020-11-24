#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define BROKER "192.168.178.199"

#define ON String(F("ON")).c_str()
#define OFF String(F("OFF")).c_str()

#define PUB(x,y) mqtt.publish(x, y)
//#define DBG(x) Serial.println(F(x))

#define ONEWIRE true
#define SDMETER true
#define PENKTH000 false // untested

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoOTA.h>
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

#if ONEWIRE
#include <OneWire.h>
#include <DallasTemperature.h>
#define OW_BUS D4
OneWire oneWire(OW_BUS);
DallasTemperature sensors(&oneWire);
unsigned long next_OW;
#endif

// ---- MQTT client --------------------------------------------------------------
#include <PubSubClient.h>
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// ---- PENKTH000 / ModBus stuff -------------------------------------------------
#if PENKTH000
#include <ModbusRTU.h>
#define SLAVE_ID  0xB0
#define REGN 8
ModbusRTU mb;

#include <SoftwareSerial.h>
#define TX  D1
#define RX  D2
#define RTS D0
SoftwareSerial RS485;//(RX, TX, false, 256);   // additional serial port for RS485
#endif

// ---- SDM / ModBus enegy meter -------------------------------------------------
#if SDMETER
#include "src/SDM/SDM.h"
#include <SoftwareSerial.h>
#define SDM_TX   D6
#define SDM_RX   D5
#define SDM_DERE D0
SoftwareSerial SDM_METER;

SDM sdm(SDM_METER, 9600, SDM_DERE, SWSERIAL_8E1, SDM_RX, SDM_TX);

unsigned long lastEnergy=0;
#endif

// ---- REMO_SLAVE / Remote stuff ("D0") -----------------------------------------
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

void mqtt_reconnect() {
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void setup() {
  /*
  pinMode(TX, INPUT);
  pinMode(RX, INPUT);
  delay(10000);
  */
  
#ifdef DBG
  Serial.begin(115200);
  Serial.println(F("AWHP begin"));
#endif

  // Init WiFi
  {
    WiFi.setOutputPower(20.5); // this sets wifi to highest power
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long timeout = millis() + 10000;

    while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
      yield();
    }

    // Reset because of no connection
    if (WiFi.status() != WL_CONNECTED) {
      hardreset();
    }
#ifdef DBG
    DBG("WIFI READY");
#endif
  }

  // Init HTTP-Update-Server
  httpUpdater.setup(&httpServer, "admin", "");
  httpServer.begin();
  MDNS.begin("AWHP");
  MDNS.addService("http", "tcp", 80);

  // Init MQTT
  mqtt.setServer(BROKER, 1883);
  mqtt.setCallback(callback);

#if PENKTH000
  RS485_Init();
#endif

#if ONEWIRE
  sensors.begin();
  sensors.setResolution(11);
  sensors.setWaitForConversion(false);  // makes it async
#endif

#if SDMETER
  sdm.begin();
#endif

  D0_Init();
}

///////////////////////////////////////////////////////////////////////////////

// see 99_loop

///////////////////////////////////////////////////////////////////////////////
