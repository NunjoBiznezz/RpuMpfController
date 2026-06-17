#include "SwitchManager.h"
#include "RPU.h"
#include "RPU_Internals.h"

namespace RPU {

volatile uint8_t SwitchManager::SwitchesMinus2[NUM_SWITCH_BYTES] = {};
volatile uint8_t SwitchManager::SwitchesMinus1[NUM_SWITCH_BYTES] = {};
volatile uint8_t SwitchManager::SwitchesNow[NUM_SWITCH_BYTES] = {};
CircularStack<uint8_t, 60, 0xFF> SwitchManager::Stack;
int SwitchManager::NumGameSwitches = 0;
int SwitchManager::NumGamePrioritySwitches = 0;
const PlayfieldAndCabinetSwitch* SwitchManager::GameSwitches = nullptr;

#if (RPU_MPU_ARCHITECTURE >= 10)
bool SwitchManager::UpDownSwitch = false;
#endif

SwitchManager& SwitchManager::getInstance() {
    static SwitchManager instance;
    return instance;
}

void SwitchManager::push(uint8_t switchNumber) {
    if (switchNumber == 0xFF) return;
    if (Stack.spaceLeft() == 0) return;
    if (switchNumber == SW_SELF_TEST_SWITCH) {
        if (!Stack.isEmpty() && Stack.peek() == SW_SELF_TEST_SWITCH) return;
    }
    Stack.push(switchNumber);
}

uint8_t SwitchManager::pull() {
    return Stack.pull();
}

bool SwitchManager::readState(uint8_t switchNum) const {
    if (switchNum >= MAX_NUM_SWITCHES) return false;
    int switchByte = switchNum / 8;
    int switchBit = switchNum % 8;
    return ((SwitchesNow[switchByte] >> switchBit) & 0x01) != 0;
}

void SwitchManager::setupGameSwitches(int numSwitches, int numPrioritySwitches,
                                       const PlayfieldAndCabinetSwitch* gameSwitches) {
    NumGameSwitches = numSwitches;
    NumGamePrioritySwitches = numPrioritySwitches;
    GameSwitches = gameSwitches;
}

#if (RPU_MPU_ARCHITECTURE < 10)
bool SwitchManager::getUpDownSwitchState() const { return true; }
void SwitchManager::clearUpDownSwitchState() {}
#else
bool SwitchManager::getUpDownSwitchState() const { return UpDownSwitch; }
void SwitchManager::clearUpDownSwitchState() { UpDownSwitch = false; }
#endif

} // namespace RPU
