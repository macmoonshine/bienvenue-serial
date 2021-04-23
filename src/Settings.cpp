#include "Settings.h"
#include <EEPROM.h>

const size_t kCheckSize = sizeof(Settings);
const int16_t kCheckPrime = 127;
const bool debug = false;

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

bool Settings::check() {
    return checksum[1] == kCheckPrime && ::checksum(this) == 0;
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
 