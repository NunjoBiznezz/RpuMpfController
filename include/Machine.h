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

    // Sound — default no-ops for machines with no native audio.
    virtual uint8_t numSounds()          const { return 0; }
    virtual void    initAudio()          const {}
    virtual void    playSound(uint8_t)   const {}
    virtual void    stopAllSounds()      const {}

    virtual ~Machine() = default;
};