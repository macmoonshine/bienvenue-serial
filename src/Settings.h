#include <Arduino.h>
#include <EEPROM.h>

#define kStringSize 32


struct Settings {
    public:
        typedef char Text[kStringSize];

        Text name;
        Text ssid;
        Text password;
        uint32_t baud;
        int8_t rx;
        int8_t tx;

        Settings(const Text name, const Text ssid, const Text password, uint32_t baud, int8_t rx = -1, int8_t tx = -1) {
            strncpy(this->name, name, sizeof(Text) - 1);
            strncpy(this->ssid, ssid, sizeof(Text) - 1);
            strncpy(this->password, password, sizeof(Text) - 1);
            this->baud = baud;
            this->rx = rx;
            this->tx = tx;
            checksum[0] = 0;
            checksum[1] = 0;
        }
        
        bool check();
        void update();
    private:
        int8_t checksum[2];
};