#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>

#define kTextSize 32
#define kNoPin -1

struct Settings {
    public:
        typedef int8_t pin;
        typedef char Text[kTextSize];

        Text name;
        Text ssid;
        Text password;
        Text mDNSService;
        uint16_t port;
        uint32_t baud;
        pin rx;
        pin tx;
        pin dtr;
        pin dsr;
        pin rts;
        pin cts;

        Settings(const Text name, const Text ssid, const Text password, const Text mDNSService,
                 uint16_t port = 2345, uint32_t baud = 115200, pin rx = kNoPin, pin tx = kNoPin,
                 pin dtr = kNoPin, pin dsr = kNoPin, pin rts = kNoPin, pin cts = kNoPin) {
            strncpy(this->name, name, sizeof(Text) - 1);
            strncpy(this->ssid, ssid, sizeof(Text) - 1);
            strncpy(this->password, password, sizeof(Text) - 1);
            strncpy(this->mDNSService, mDNSService, sizeof(Text) - 1);
            this->baud = baud;
            this->rx = rx;
            this->tx = tx;
            this->dtr = dtr;
            this->dsr = dsr;
            this->rts = rts;
            this->cts = cts;
            checksum[0] = 0;
            checksum[1] = 0;
        }

        bool useDefaultSerial() {
            return rx < 0;
        }
        
        bool check() const;
        void clear();
        void update();
        void applyTxtRecord(MDNSResponder::hMDNSService service) const;

        void print(Print &out) const;
    private:
        int8_t checksum[2];
};