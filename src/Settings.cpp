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

        if(debug && i > 96) {
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
    return checksum[1] == kCheckPrime && ::checksum(this) == 0;
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
    dtr = kNoPin;
    dsr = kNoPin;
    rts = kNoPin;
    cts = kNoPin;
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
    MDNS.addServiceTxt(service, "baud", itoa(baud, buffer, 10));
    MDNS.addServiceTxt(service, "port", itoa(port, buffer, 10));
    MDNS.addServiceTxt(service, "rx", itoa(rx, buffer, 10));
    MDNS.addServiceTxt(service, "tx", itoa(tx, buffer, 10));
}

void Settings::print(Print &out) const {
    _print(out, 'n', "Name", name);
    _print(out, 's', "SSID", ssid);
    _print(out, 'P', "Password", "******");
    _print(out, 'o', "Mode", kWifiModes[wifiMode & 0x3]);
    _print(out, 'a', "IP Address", ipAddress().toString());
    _print(out, 'm', "mDNS Service", mDNSService);
    _print(out, 'p', "Port", port);
    _print(out, 'b', "Baud", baud);
    _print(out, 'r', "RX", rx, false);
    _print(out, 't', "TX", tx);
    _print(out, '1', "DTR", dtr, false);
    _print(out, '2', "DSR", dsr);
    _print(out, '3', "RTS", rts, false);
    _print(out, '4', "CTS", cts);
}