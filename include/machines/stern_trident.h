#pragma once
#include "../Machine.h"

// Stern Trident (1979) on Bally/Stern 2517 MPU (M100).
// RPU feature flags (RPU_OS_USE_DIP_SWITCHES, RPU_OS_USE_SB100) are set in
// platformio.ini under [mpf_controller_app] so they reach all translation units.
class SternTrident : public Machine {
public:
    uint8_t numSwitches()              const override { return 36; }
    uint8_t numSolenoids()             const override { return 15; }
    uint8_t numLamps()                 const override { return 60; }
    uint8_t numDisplays()              const override { return 6;  }
    uint8_t numModernLights()          const override { return 0;  }
    uint8_t displayType(uint8_t)       const override { return 3;  } // LISY_DISPLAY_TYPE_7SEG
    uint8_t displayDigits(uint8_t)     const override { return 6;  }
};
