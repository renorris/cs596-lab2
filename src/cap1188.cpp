#include "cap1188.h"

bool setCapSensitivity(Adafruit_CAP1188* cap, int sensitivity) {
    // Sensitivity value has 3 bits of resolution
    if (sensitivity < 0 || sensitivity > 0b111) {
        Serial.println("sensitivity value out-of-range");
        return false;
    }

    // Given a byte where the LSB is `B0` and the MSB is `B7`,
    // the 3-bit sensitivity value must be placed into B6, B5 
    // and B4 of the destination register.
    uint8_t val = uint8_t(sensitivity) << 4;

    // Write the value
    const uint8_t sensitivityControlRegisterAddress = 0x1F;
    (*cap).writeRegister(sensitivityControlRegisterAddress, val);
    
    return true;
}

int getNumTouched(Adafruit_CAP1188* cap) {
    uint8_t raw = (*cap).touched();
    int numTouched = 0;
    for (int i = 0; i < CHAR_BIT; i++) {
        if ((raw >> i) & 1) {
            numTouched++;
        }
    }
    return numTouched;
}