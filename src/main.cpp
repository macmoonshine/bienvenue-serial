/*
Copyright 2021 macoonshine

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>
#include "Settings.h"

const Settings::Text kService = "cnc";
const uint16_t kPort = 2345;
const uint32_t kBaudRate = 115200;

#define LED1 D4
#define LED2 D3
#define RESET D8

int modePin = LOW;
Settings settings("", "", "", kService, D5, kPort, kBaudRate);
WiFiServer server(kPort);
SoftwareSerial softwareSerial;
IPAddress accessPointNetMask(255, 255, 255, 0);

Stream *serialStream = NULL;

void readSettings() {
    EEPROM.get(0, settings);
}

bool writeSettings() {
    EEPROM.put(0, settings);
    return EEPROM.commit();
}

bool startWLAN(Print *out = NULL) {
#ifdef ACCESS_POINT
    if(settings.isServer()) {
        IPAddress accessPointIP = settings.ipAddress();

        return WiFi.mode(settings.wifiMode) &&
            WiFi.softAPConfig(accessPointIP, accessPointIP, accessPointNetMask);
            WiFi.softAP(settings.ssid, settings.password);
    }
    else {
        WiFi.mode(WIFI_STA);
        WiFi.begin(settings.ssid, settings.password);
    }
#else
    WiFi.begin(settings.ssid, settings.password);
#endif
    if(out) {
        out->println("");
        out->print("Connecting to WiFi(");
        out->print(settings.ssid);
        out->print(")");
    }
    for(int i = 0; i < 32 && !WiFi.isConnected(); ++i) {
        if(out) {
            out->print(".");
        }
        delay(1000);
    }
    if(out) {
        out->println("");
    }
    if(WiFi.isConnected()) {
        if(out) {
            out->print("IP: ");
            out->println(WiFi.localIP());
        }
        if(MDNS.begin(settings.name)) {
            if(out) {
                out->print("Service: ");
                out->print(settings.name);
                out->println(".local");
            }
            return true;
        }
        else if(out) {
            out->println("Can't init mDNS.");
        }
    }
    else if(out) {
        out->println("Can't connect to WLAN.");
    }
    return false;
}

void clearStream(Stream &stream) {
    while(stream.available()) {
        stream.read();
    }
}

void enterText(Stream &stream, const char *label, Settings::Text value, bool secure = false) {
    int index = 0;
    const int size = sizeof(Settings::Text);

    clearStream(stream);
    stream.print(label);
    stream.flush();
    while(index < size - 1) {
        if(stream.available()) {
            char c = stream.read();

            switch (c) {
                case '\b':
                    if(index > 0) {
                        stream.print(c);
                        stream.flush();
                        --index;
                    }
                    break;
                case '\n':
                case '\r':
                    value[index] = 0;
                    index = size;
                    break;
                default:
                    stream.print(secure ? '*' : c);
                    stream.flush();
                    value[index++] = c;
            }
        }
        else {
            delay(200);
        }
    }
    value[size - 1] = 0;
    stream.println();
}

int enterInt(Stream &stream, const char *label, int length = 10) {
    int index = 0;
    int value = 0;
    int sign = 1;

    clearStream(stream);
    stream.print(label);
    stream.flush();
    while(index < length) {
        if(stream.available()) {
            char c = stream.read();

            switch (c) {
                case '\b':
                    if(index > 0) {
                        stream.print(c);
                        stream.flush();
                        --index;
                        value /= 10;
                    }
                    break;
                case '\n':
                case '\r':
                    return sign * value;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    stream.print(c);
                    stream.flush();
                    value *= 10;
                    value += c - '0';
                    ++index;
                    break;
                case '-':
                    if(index == 0) {
                        stream.print(c);
                        stream.flush();
                        sign = -1;
                    }
                    break;
                default:
                    break;
            }
        }
        else {
            delay(200);
        }
    }
    stream.println();
    return sign * value;
}

WiFiMode_t enterMode(Stream &stream) {
    const char *separator = " (";
    clearStream(stream);
    stream.print("Wifi Mode");
    for(int i = 0; i < 4; ++i) {
        stream.print(separator);
        stream.print(i);
        stream.print(". ");
        stream.print(kWifiModes[i]);
        separator = ", ";
    }
    stream.print("): ");
    stream.flush();
    while(true) {
        if(stream.available()) {
            char c = stream.read();

            switch (c) {
                case '\n':
                case '\r':
                    stream.println();
                    return WIFI_OFF;
                case '0':
                case '1':
                case '2':
                case '3':
                    stream.println(c);
                    return WiFiMode_t(c - '0');
                default:
                    break;
            }
        }
        else {
            delay(200);
        }
    }
    return WIFI_OFF;
}

void disconnect() {
    MDNS.end();
    WiFi.disconnect(true);
}

bool checkSettings(Print &out) {
    if(startWLAN(&out)) {
        disconnect();
        settings.update();
        return settings.check();
    }
    else {
        disconnect();
        return false;
    }
}

void checkAndWriteSettings(Print &out) {
    if(checkSettings(out)) {
        out.println("Saving values into eeprom.");
        if(writeSettings()) {
            out.println("Settings written.");
        }
        else {
            out.println("Settings not written.");
        }
    }
    else {
        out.println("Can't create checksum.");
    }
}

void configure(Stream &stream) {
    bool redraw = true;

    clearStream(stream);
    while(true) {
        if(redraw) {
            settings.print(stream);
            stream.println("q) Quit   C) Check WLAN   R) Read   W) Write   X) Clear");
            stream.println();
            redraw = false;
        }
        if(stream.available()) {
            char key = stream.read();

            redraw = true;
            switch (key) {
                case '\r':
                case '\n':
                case ' ':
                    break;
                case '1':
                    settings.dtr = enterInt(stream, "DTR: ", 2);
                    break;
                case '2':
                    settings.dsr = enterInt(stream, "DSR: ", 2);
                    break;
                case '3':
                    settings.rts = enterInt(stream, "RTS: ", 2);
                    break;
                case '4':
                    settings.cts = enterInt(stream, "CTS: ", 2);
                    break;
#ifdef ACCESS_POINT
                case 'a': {
                    char buffer[128];
                    IPAddress ipAddress(0);

                    enterText(stream, "IP Address: ", buffer);
                    if(ipAddress.fromString(buffer)) {
                        settings.setIPAddress(ipAddress);
                    }
                    break;
                }
                case 'd':
                    settings.wifiMode = enterMode(stream);
                    break;
#endif
                case 'b':
                    settings.baud = enterInt(stream, "Baud Rate: ");
                    break;
                case 'm':
                    enterText(stream, "mDNS Service: ", settings.mDNSService);
                    break;
                case 'n':
                    enterText(stream, "Name: ", settings.name);
                    break;
                case 'o':
                    settings.modePin = enterInt(stream, "Mode Pin: ", 2);
                    break;
                case 'p':
                    settings.port = enterInt(stream, "Port: ", 5);
                    break;
                case 'q':
                    stream.println("Quit settings...");
                    return;
                case 'r':
                    settings.rx = enterInt(stream, "Rx Pin: ", 2);
                    break;
                case 's':
                    enterText(stream, "SSID: ", settings.ssid);
                    break;
                case 't':
                    settings.tx = enterInt(stream, "Tx Pin: ", 2);
                    break;
                case 'C':
                    checkSettings(stream);
                    stream.println();
                    break;
                case 'P':
                    enterText(stream, "Password: ", settings.password, true);
                    break;
                case 'R':
                    stream.println("Read settings...");
                    readSettings();
                    break;
                case 'W':
                    checkAndWriteSettings(stream);
                    stream.println();
                    break;
                case 'X':
                    settings.clear();
                    settings.port = kPort;
                    settings.baud = kBaudRate;
                    break;
                default:
                    redraw = false;
                    break;
            }
        }
        else {
            delay(200);
        }
    }
}

void waitForRestart(Stream &stream) {
    stream.println("Please restart controller");
    while(true) {
        digitalWrite(LED2, HIGH);
        delay(1000);
        digitalWrite(LED2, LOW);
        delay(1000);
    }
}

void setupPins() {
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    if(settings.checkModePin()) {
        pinMode(settings.modePin, INPUT);
    }
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    if(settings.dtr >= 0) {
        pinMode(settings.dtr, OUTPUT);
        digitalWrite(settings.dtr, HIGH);
    }
}

void setup() {
    Stream &stream = Serial;

    EEPROM.begin(sizeof(settings));
    readSettings();
    setupPins();
    Serial.begin(kBaudRate);
    delay(1000);
    if(settings.check() && startWLAN()) {
        MDNSResponder::hMDNSService service = MDNS.addService(settings.name, "cnc", "tcp", settings.port);

        digitalWrite(LED1, HIGH);
        MDNS.addServiceTxt(service, "model", "esp8266");
        settings.applyTxtRecord(service);
        server.begin();
        digitalWrite(LED2, HIGH);
        if(settings.checkModePin()) {
            modePin = digitalRead(settings.modePin);
        }
        serialStream = NULL;
        if(settings.useDefaultSerial()) {
            Serial.end();
        }
    }
    else {
        stream.println("Invalid settings");
        configure(stream);
        waitForRestart(stream);
    }
}

void sendReset() {
    if(settings.dtr >= 0) {
        digitalWrite(settings.dtr, LOW);
        delay(10);
        digitalWrite(settings.dtr, HIGH);
        delay(10);
    }
}

void setupSerial() {
    sendReset();
    if(settings.useDefaultSerial()) {
        Serial.end();
        Serial.begin(settings.baud);
        serialStream = &Serial;
    }
    else {
        softwareSerial.begin(settings.baud, SWSERIAL_8N1, settings.rx, settings.tx, false);
        serialStream = &softwareSerial;
    }
}

void transfer(Stream &input, Stream &output) {
    if(input.available()) {
        digitalWrite(LED2, LOW);
        while(input.available()) {
            char c = input.read();

            output.write(c);
        }
        output.flush();
        digitalWrite(LED2, HIGH);
    }
}

void handleClient(WiFiClient &client) {
    setupSerial();
    while(client.connected() && serialStream) {
        transfer(*serialStream, client);
        transfer(client, *serialStream);
    }
    if(serialStream) {
        softwareSerial.end();
    }
    serialStream = NULL;
}

void loop() {
    MDNS.update();

    WiFiClient client = server.available();

    if(client.connected()) {
        digitalWrite(LED1, LOW);
        handleClient(client);
        digitalWrite(LED1, HIGH);
    }
    else if(settings.checkModePin() && modePin != digitalRead(settings.modePin)) {
        disconnect();
        Serial.begin(settings.baud);
        configure(Serial);
        waitForRestart(Serial);
    }
    else if(!settings.useDefaultSerial() && Serial.available() &&
        Serial.read() == 'x') {
        configure(Serial);
        waitForRestart(Serial);
    }
    delay(500);
}