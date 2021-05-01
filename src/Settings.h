#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>

#define kTextSize 32


struct Settings {
    public:
        typedef char Text[kTextSize];

        Text name;
        Text ssid;
        Text password;
        Text mDNSService;
        uint16_t port;
        uint32_t baud;
        int8_t rx;
        int8_t tx;

        Settings(const Text name, const Text ssid, const Text password, const Text mDNSService,
                 uint16_t port = 2480, uint32_t baud = 115200, int8_t rx = -1, int8_t tx = -1) {
            strncpy(this->name, name, sizeof(Text) - 1);
            strncpy(this->ssid, ssid, sizeof(Text) - 1);
            strncpy(this->password, password, sizeof(Text) - 1);
            strncpy(this->mDNSService, mDNSService, sizeof(Text) - 1);
            this->baud = baud;
            this->rx = rx;
            this->tx = tx;
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