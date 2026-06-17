#ifndef SWITCH_MANAGER_H
#define SWITCH_MANAGER_H

#include <stdint.h>
#include "RPU_config.h"
#include "CircularStack.h"


struct PlayfieldAndCabinetSwitch;

class SwitchManager {
public:
    static SwitchManager& getInstance();

    void    push(uint8_t switchNumber);
    uint8_t pull();
    bool    readState(uint8_t switchNum) const;
    void    setupGameSwitches(int numSwitches, int numPrioritySwitches,
                              const PlayfieldAndCabinetSwitch* gameSwitches);
    bool    getUpDownSwitchState() const;
    void    clearUpDownSwitchState();

    // ISR-accessible switch matrix state
    static volatile uint8_t* getSwitchesNow()    { return SwitchesNow; }
    static volatile uint8_t* getSwitchesMinus1() { return SwitchesMinus1; }
    static volatile uint8_t* getSwitchesMinus2() { return SwitchesMinus2; }

private:
    SwitchManager() = default;
    ~SwitchManager() = default;
    SwitchManager(const SwitchManager&) = delete;
    SwitchManager& operator=(const SwitchManager&) = delete;

    static volatile uint8_t SwitchesMinus2[NUM_SWITCH_BYTES];
    static volatile uint8_t SwitchesMinus1[NUM_SWITCH_BYTES];
    static volatile uint8_t SwitchesNow[NUM_SWITCH_BYTES];

    static CircularStack<uint8_t, 60, 0xFF> Stack;

    static int NumGameSwitches;
    static int NumGamePrioritySwitches;
    static const PlayfieldAndCabinetSwitch* GameSwitches;

#if (RPU_MPU_ARCHITECTURE >= 10)
    static bool UpDownSwitch;
#endif
};


#endif // SWITCH_MANAGER_H
