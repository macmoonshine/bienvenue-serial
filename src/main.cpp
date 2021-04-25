#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include "Settings.h"

const uint16_t kPort = 2345;

Settings settings("", "", "", 115200);
WiFiServer server(kPort);

void readSettings() {
    EEPROM.get(0, settings);
}

bool writeSettings() {
    EEPROM.put(0, settings);
    return EEPROM.commit();
}

bool connect(bool debug) {
    WiFi.begin(settings.ssid, settings.password);
    if(debug) {
        Serial.println("");
        Serial.print("Connecting to WiFi(");
        Serial.print(settings.ssid);
        Serial.print(")");
    }
    for(int i = 0; i < 128 && WiFi.status() != WL_CONNECTED; ++i) {
        if(debug) {
            Serial.print(".");
        }
        delay(1000);
    }
    if(WiFi.status() == WL_CONNECTED) {
        if(debug) {
            Serial.println("");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
        }
        if(MDNS.begin(settings.name)) {
            if(debug) {
                Serial.print("Service: ");
                Serial.print(settings.name);
                Serial.println(".local");
            }
            return true;
        }
        else if(debug) {
            Serial.println("Can't init mDNS.");
        }
    }
    return false;
}

void enterText(const char *label, Settings::Text value) {
    int index = 0;
    const int size = sizeof(Settings::Text);

    while(Serial.available()) {
        Serial.read();
    }
    Serial.print(label);
    Serial.flush();
    while(index < size - 1) {
        if(Serial.available()) {
            char c = Serial.read();

            switch (c) {
            case '\n':
            case '\r':
                value[index] = 0;
                index = size;
            default:
                Serial.print(c);
                Serial.flush();
                value[index++] = c;
            }
        }
        else {
            delay(200);
        }
    }
    value[size - 1] = 0;
    Serial.println();
}

void enterWifi() {
    delay(1000);
    Serial.println("");
    enterText("Name    : ", settings.name);
    enterText("SSID    : ", settings.ssid);
    enterText("Password: ", settings.password);
    if(connect(true)) {
        settings.update();
        if(settings.check()) {
            Serial.println("Saving values into eeprom.");
            if(writeSettings()) {
                Serial.println("Settings written.");
            }
            else {
                Serial.println("Settings not written.");
            }
        }
        else {
            Serial.println("Can't create checksum.");
        }
    }
}

void setup() {
    Serial.begin(115200);
    EEPROM.begin(sizeof(settings));
    readSettings();
    if(settings.check() && connect(false)) {
        MDNS.addService(settings.name, "rxtx", "tcp", kPort);
        server.begin();
    }
    else {
        Serial.println("Invalid settings.");
        enterWifi();
        Serial.println("Please restart controller.");
        while(true) {
            delay(1000);
        }
    }
}

void transfer(Stream &input, Stream &output) {
    while(input.available()) {
        char c = input.read();

        output.write(c);
    }
    output.flush();
}

void handleClient(WiFiClient &client) {
    while(client.connected()) {
        transfer(client, Serial);
        transfer(Serial, client);
    }
}

void loop() {
    MDNS.update();

    WiFiClient client = server.available();

    if(client.connected()) {
        handleClient(client);
    }
    delay(500);
}