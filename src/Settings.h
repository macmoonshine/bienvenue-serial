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
#include <ESP8266mDNS.h>
#include <ESP8266WiFiType.h>
#include <IPAddress.h>

#define kTextSize 32
#define kNoPin -1

extern const char *kWifiModes[];

struct Settings {
    public:
        typedef int8_t pin;
        typedef char Text[kTextSize];

        Text name;
        Text ssid;
        Text password;
        Text mDNSService;
        WiFiMode_t wifiMode;
        uint32_t address; 
        uint16_t port;
        uint32_t baud;
        pin rx;
        pin tx;
        pin reset;
        pin modePin;
        pin reserved[2]; // needed for alignment
        int8_t checksum[2];

        Settings(const Text name, const Text ssid, const Text password, const Text mDNSService,
                 uint16_t port = 2345, uint32_t baud = 115200,
                 pin rx = kNoPin, pin tx = kNoPin, pin reset = D5, pin modePin = D0) {
            strncpy(this->name, name, sizeof(Text) - 1);
            strncpy(this->ssid, ssid, sizeof(Text) - 1);
            strncpy(this->password, password, sizeof(Text) - 1);
            strncpy(this->mDNSService, mDNSService, sizeof(Text) - 1);
            this->baud = baud;
            this->rx = rx;
            this->tx = tx;
            this->reset = reset;
            this->modePin = modePin;
            checksum[0] = 0;
            checksum[1] = 0;
        }

        const IPAddress ipAddress() const {
            return IPAddress(address);
        }

        bool setIPAddress(const IPAddress &address) {
            if(address.isV4()) {
                this->address = address;
                return true;
            }
            else {
                return false;
            }
        }

        bool useDefaultSerial() const {
            return rx < 0;
        }
        bool isServer() const {
            return (wifiMode & WIFI_AP) != 0;
        }
        bool checkModePin() const {
            return modePin != kNoPin;
        }
        
        bool check() const;
        void clear();
        void update();
        void applyTxtRecord(MDNSResponder::hMDNSService service) const;

        void print(Print &out) const;
};