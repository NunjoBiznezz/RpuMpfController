#pragma once
#include <stdint.h>

class Machine {
    uint32_t solenoidOnMask = 0;

public:
    // ── Resource counts (machine must implement) ───────────────────────────
    virtual uint8_t numSwitches()              const = 0;
    virtual uint8_t numSolenoids()             const = 0;
    virtual uint8_t numLamps()                 const = 0;
    virtual uint8_t numDisplays()              const = 0;
    virtual uint8_t numModernLights()          const = 0;
    virtual uint8_t displayType(uint8_t idx)   const = 0;
    virtual uint8_t displayDigits(uint8_t idx) const = 0;

    // ── Lifecycle ──────────────────────────────────────────────────────────
    virtual void setup();
    virtual void loop(unsigned long now);

    // ── Switches ───────────────────────────────────────────────────────────
    // Dequeues one switch-change event encoded as the LISY byte
    // (bit7=state, bits0-6=switchNum). Returns false if no events pending.
    bool    getNextSwitchChange(uint8_t &lisyByte);
    uint8_t getSwitchState(uint8_t n) const;

    // ── Hardware writes ────────────────────────────────────────────────────
    virtual void setLamp(uint8_t n, bool state);
    virtual void fireSolenoid(uint8_t n);
    virtual void setSolenoidOn(uint8_t n);
    virtual void setSolenoidOff(uint8_t n);
    virtual void setDisplay(uint8_t idx, unsigned long value, uint8_t blankMask);

    // ── Sound ─────────────────────────────────────────────────────────────
    virtual uint8_t numSounds()          const;
    virtual void    playSound(uint8_t n) const;
    virtual void    stopAllSounds()      const;

    virtual ~Machine() = default;
};
