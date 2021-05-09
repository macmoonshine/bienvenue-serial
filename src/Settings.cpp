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

#include "Settings.h"
#include <EEPROM.h>

const size_t kCheckSize = sizeof(Settings);
const int16_t kCheckPrime = 127;
const bool debug = false;
const char *kWifiModes[] = { "Off", "Station", "Access Point", "Station & Access Point" };

void _spaces(Print &out, int count) {
    for(int i = 0; i < count; ++i) {
        out.print(" ");
    }
}
template<class T>
void _print(Print &out, char key, const char *title, T value, bool newline = true) {
    out.print(key);
    out.print(") ");
    out.print(title);
    _spaces(out, 12 - strlen(title));
    out.print(": ");
    if(newline) {
        out.println(value);
    }
    else {
        _spaces(out, 16 - out.print(value));
    }
}

static int8_t checksum(const void *buffer, const size_t size = kCheckSize) {
    int16_t sum = 0;

    for(size_t i = 0; i < size; ++i) {
        const int16_t sign = 1 + (i % 2);
        const int16_t value = static_cast<const char *>(buffer)[i];

        if(debug && i > size - 8) {
            Serial.print(i);
            Serial.print(": ");
            Serial.print(sign);
            Serial.print(" * ");
            Serial.print(value);
            Serial.print(" + ");
            Serial.println(sum);
        }
        sum = (sum + sign * value) % kCheckPrime;
    }
    return static_cast<int8_t>(sum);
}

bool Settings::check() const {
    const int8_t sum = ::checksum(this);

    if(debug) {
        Serial.print("check(): ");
        Serial.print((int) checksum[1]);
        Serial.print(", ");
        Serial.println((int) sum);
    }
    return checksum[1] == kCheckPrime && sum == 0;
}

void Settings::clear() {
    memset(name, 0, kTextSize);
    memset(ssid, 0, kTextSize);
    memset(password, 0, kTextSize);
    memset(mDNSService, 0, kTextSize);
    wifiMode = WIFI_STA;
    address = 0;
    port = 2345;
    baud = 9600;
    rx = kNoPin;
    tx = kNoPin;
    reset = D0;
    modePin = D0;
}

void Settings::update() {
    const int8_t sum = -::checksum(this, kCheckSize - sizeof(checksum));

    checksum[0] = sum < 0 ? kCheckPrime + sum : sum;
    checksum[1] = kCheckPrime;
    if(debug) {
        Serial.print("sum = ");
        Serial.print(-sum);
        Serial.print(", checksum = ");
        Serial.println(checksum[0]);
        Serial.println(::checksum(this));
    }
} 

void Settings::applyTxtRecord(MDNSResponder::hMDNSService service) const {
    Text buffer;

    MDNS.addServiceTxt(service, "ssid", ssid);
    MDNS.addServiceTxt(service, "modePin", modePin);
    MDNS.addServiceTxt(service, "baud", itoa(baud, buffer, 10));
    MDNS.addServiceTxt(service, "port", itoa(port, buffer, 10));
    MDNS.addServiceTxt(service, "rx", itoa(rx, buffer, 10));
    MDNS.addServiceTxt(service, "tx", itoa(tx, buffer, 10));
    MDNS.addServiceTxt(service, "reset", itoa(reset, buffer, 10));
}

void Settings::print(Print &out) const {
    _print(out, 'n', "Name", name);
    _print(out, 's', "SSID", ssid);
    _print(out, 'P', "Password", strlen(password) == 0 ? "" : "******");
#ifdef ACCESS_POINT
    _print(out, 'd', "Mode", kWifiModes[wifiMode & 0x3]);
    _print(out, 'a', "IP Address", ipAddress().toString());
#endif
    _print(out, 'm', "mDNS Service", mDNSService);
    _print(out, 'p', "Port", port);
    _print(out, 'b', "Baud", baud);
    _print(out, 'r', "RX", rx);
    _print(out, 't', "TX", tx);
    _print(out, 'x', "Reset", reset);
    _print(out, 'o', "Mode Pin", modePin);
}