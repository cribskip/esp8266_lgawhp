// ----------------------------------------------------------------------------------

void loop() {
  // WiFi is important
  if (WiFi.status() != WL_CONNECTED) ESP.restart();
  if (!mqtt.connected()) mqtt_reconnect();
  httpServer.handleClient();
  _yield();

  handle_D0();

#if ONEWIRE
  handle_onewire();
#endif

  handle_D0();

#if SDMETER
  if (millis() - lastEnergy  > 5000) {
    mqtt.publish("AWHP/energy/W", String(sdm.readVal(DDM_PHASE_1_POWER)).c_str());

    handle_D0();

    mqtt.publish("AWHP/energy/kWh", String(sdm.readVal(DDM_IMPORT_ACTIVE_ENERGY)).c_str());
    lastEnergy = millis();
  }
#endif

  // Watchdog reset if there is no communication
  if (state.sent == true && ((millis() - lastCharTime) > 60000 || (millis() - lastMsgTime) > 60000)) {
    wdt_enable(WDTO_15MS);
    Serial.println(F("RX TIMEOUT, WDT RESET!!"));
    mqtt.publish("AWHP/debug", String(F("WDT RESET")).c_str());
    _yield();

    while (true) {
      ;;;
    }
  }
}
