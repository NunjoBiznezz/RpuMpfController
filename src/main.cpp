#include <Arduino.h>
#include "RPU.h"
#include "RPU_config.h"
#include "lisy_protocol.h"


constexpr uint32_t SERIAL_BAUD = 115200;

// LISY type-3 (7-segment) bit encoding per the protocol spec:
//   bit7=dp, bit6=g, bit5=f, bit4=e, bit3=d, bit2=a, bit1=b, bit0=c
// Returns the BCD digit (0-9) for a recognizable segment pattern, or 0xFF for blank/unknown.
static uint8_t segToBcd(uint8_t seg) {
    static const uint8_t kTable[10] = {
        0x3F, // 0: a,b,c,d,e,f
        0x03, // 1: b,c
        0x5B, // 2: a,b,d,e,g
        0x4F, // 3: a,b,c,d,g
        0x63, // 4: b,c,f,g
        0x6D, // 5: a,c,d,f,g
        0x7D, // 6: a,c,d,e,f,g
        0x07, // 7: a,b,c
        0x7F, // 8: all segments
        0x6F, // 9: a,b,c,d,f,g
    };
    seg &= 0x7F; // strip decimal-point bit
    for (uint8_t i = 0; i < 10; i++) {
        if (kTable[i] == seg) return i;
    }
    return 0xFF;
}

// Block until a byte arrives, then return it.
static uint8_t readByte() {
    while (!Serial.available());
    return (uint8_t)Serial.read();
}

// Handle a LISY display command (command byte already consumed).
// Reads [byte_count, seg_byte…] from serial, decodes segment bytes to BCD,
// and drives the RPU display at displayIdx.
static void handleDisplayCmd(uint8_t displayIdx) {
    uint8_t byteCount = readByte();

    unsigned long value    = 0;
    uint8_t       blankMask = 0;

    for (uint8_t i = 0; i < byteCount; i++) {
        uint8_t seg = readByte();
        if (i < MACHINE_DISPLAY_DIGITS) {
            uint8_t bcd = segToBcd(seg);
            bool    lit = (bcd <= 9);
            value     = value * 10 + (lit ? bcd : 0);
            blankMask = (uint8_t)((blankMask << 1) | (lit ? 1 : 0));
        }
    }

    if (displayIdx < MACHINE_NUM_DISPLAYS) {
        RPU_SetDisplay(displayIdx, value, false, 1);
        RPU_SetDisplayBlank(displayIdx, blankMask);
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    RPU_InitializeMPU(RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN);
}

void loop() {
    RPU_ApplyFlashToLamps(millis());

    if (!Serial.available()) return;

    uint8_t cmd = (uint8_t)Serial.read();

    // Display commands occupy a contiguous range: 30 + display_index.
    if (cmd >= LISY_DISPLAY_0 && cmd < LISY_DISPLAY_0 + MACHINE_NUM_DISPLAYS) {
        handleDisplayCmd(cmd - LISY_DISPLAY_0);
        return;
    }

    switch (cmd) {

        // ── Startup handshake ─────────────────────────────────────────────
        case LISY_RESET:
            Serial.write(0x00);
            break;

        case LISY_INFO_GET_HARDWARE:
            Serial.print(F("LISY80"));
            Serial.write(0x00);
            break;

        case LISY_INFO_LISY_VERSION:
            Serial.print(F("4.02"));
            Serial.write(0x00);
            break;

        case LISY_INFO_GET_API_VERSION:
            Serial.print(F("0.9"));
            Serial.write(0x00);
            break;

        case LISY_INFO_GET_NUM_LAMPS:
            Serial.write((uint8_t)MACHINE_NUM_LAMPS);
            break;

        case LISY_INFO_GET_NUM_SOLENOIDS:
            Serial.write((uint8_t)MACHINE_NUM_SOLENOIDS);
            break;

        case LISY_INFO_GET_NUM_DISPLAYS:
            Serial.write((uint8_t)MACHINE_NUM_DISPLAYS);
            break;

        case LISY_INFO_GET_SWITCH_COUNT:
            Serial.write((uint8_t)MACHINE_NUM_SWITCHES);
            break;

        case LISY_GET_MODERN_LIGHTS_COUNT:
            Serial.write(0x00); // no RGB LEDs on Trident
            break;

        case LISY_INFO_GET_DISPLAY_DETAILS: {
            uint8_t idx = readByte();
            if (idx < MACHINE_NUM_DISPLAYS) {
                Serial.write((uint8_t)MACHINE_DISPLAY_TYPE);
                Serial.write((uint8_t)MACHINE_DISPLAY_DIGITS);
            }
            break;
        }

        // ── Switches (stubbed — always report not present / no change) ────
        case LISY_SWITCH_GET_STATUS:
            readByte(); // consume switch number
            Serial.write(2); // 2 = not present
            break;

        case LISY_SWITCH_GET_CHANGED:
            Serial.write(0x7F); // no changes
            break;

        // ── Lamps ─────────────────────────────────────────────────────────
        case LISY_LAMP_ON:
            RPU_SetLampState(readByte(), 1);
            break;

        case LISY_LAMP_OFF:
            RPU_SetLampState(readByte(), 0);
            break;

        // ── Watchdog (stubbed — no timeout action) ────────────────────────
        case LISY_WATCHDOG:
            Serial.write(0x00);
            break;

        default:
            break; // silently ignore unknown / out-of-scope commands
    }
}
