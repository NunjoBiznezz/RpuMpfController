#pragma once
#include <stdint.h>

class Machine {
public:
    virtual uint8_t numSwitches()              const = 0;
    virtual uint8_t numSolenoids()             const = 0;
    virtual uint8_t numLamps()                 const = 0;
    virtual uint8_t numDisplays()              const = 0;
    virtual uint8_t numModernLights()          const = 0;
    virtual uint8_t displayType(uint8_t idx)   const = 0;
    virtual uint8_t displayDigits(uint8_t idx) const = 0;
    virtual ~Machine() = default;
};