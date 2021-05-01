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

#define LED1 D3
#define LED2 D4
#define MODE D5

int modePin = LOW;
Settings settings("", "", "", kService, kPort, kBaudRate);
WiFiServer server(kPort);
SoftwareSerial softwareSerial;

Stream *serialStream = NULL;

void readSettings() {
    EEPROM.get(0, settings);
}

bool writeSettings() {
    EEPROM.put(0, settings);
    return EEPROM.commit();
}

bool startWLAN(Print *out = NULL) {
    WiFi.begin(settings.ssid, settings.password);
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
            stream.println("C) Check WLAN Q) Quit R) Read W) Write X) Clear");
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
                case 'b':
                    settings.baud = enterInt(stream, "Baud Rate: ");
                    break;
                case 'm':
                    enterText(stream, "mDNS Service: ", settings.mDNSService);
                    break;
                case 'n':
                    enterText(stream, "Name: ", settings.name);
                    break;
                case 'p':
                    settings.port = enterInt(stream, "Port: ", 5);
                    break;
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
                case 'Q':
                    stream.println("Quit settings...");
                    return;
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

void setup() {
    Stream &stream = Serial;

    EEPROM.begin(sizeof(settings));
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(MODE, INPUT);
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    readSettings();
    Serial.begin(kBaudRate);
    delay(1000);
    if(settings.check() && startWLAN()) {
        MDNSResponder::hMDNSService service = MDNS.addService(settings.name, "cnc", "tcp", settings.port);

        digitalWrite(LED1, HIGH);
        MDNS.addServiceTxt(service, "model", "esp8266");
        settings.applyTxtRecord(service);
        server.begin();
        digitalWrite(LED2, HIGH);
        modePin = digitalRead(MODE);
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
    else {
        stream.println("Invalid settings");
        configure(stream);
        waitForRestart(stream);
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
    while(client.connected() && serialStream) {
        transfer(client, *serialStream);
        transfer(*serialStream, client);
    }
}

void loop() {
    MDNS.update();

    WiFiClient client = server.available();

    if(client.connected()) {
        digitalWrite(LED1, LOW);
        handleClient(client);
        digitalWrite(LED1, HIGH);
    }
    else if(modePin != digitalRead(MODE)) {
        disconnect();
        configure(Serial);
        waitForRestart(Serial);
    }
    delay(500);
}