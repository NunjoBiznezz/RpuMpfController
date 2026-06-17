#include <Arduino.h>
#include "lisy_protocol.h"

// Stub entry points. RPU hardware integration and protocol implementation
// to be added when RPU v102 board code is brought in.

constexpr uint32_t SERIAL_BAUD = 115200;
void setup() {
    Serial.begin(SERIAL_BAUD);
}

void loop() {
}
