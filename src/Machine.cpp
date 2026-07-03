#include "Machine.h"
#include "RPU.h"

void Machine::setup() {
    RPU_InitializeMPU(RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN);
    RPU_InitNativeAudio();
}

void Machine::loop(unsigned long now) {
    RPU_ApplyFlashToLamps(now);
    for (uint8_t i = 0; i < numSolenoids() && i < 32; i++) {
        if (solenoidOnMask & (1u << i)) RPU_PushToSolenoidStack(i, 1);
    }
}

bool Machine::getNextSwitchChange(uint8_t &lisyByte) {
    uint8_t sw = RPU_PullFirstFromSwitchStack();
    if (sw == SWITCH_STACK_EMPTY || sw >= numSwitches()) return false;
    lisyByte = (RPU_ReadSingleSwitchState(sw) ? 0x80 : 0x00) | (sw & 0x7F);
    return true;
}

uint8_t Machine::getSwitchState(uint8_t n) const {
    if (n >= numSwitches()) return 2; // 2 = doesn't exist
    return RPU_ReadSingleSwitchState(n) ? 1 : 0;
}

void Machine::setLamp(uint8_t n, bool state)  { RPU_SetLampState(n, state); }
void Machine::fireSolenoid(uint8_t n)          { RPU_PushToSolenoidStack(n, 1); }
void Machine::setSolenoidOn(uint8_t n)         { if (n < 32) solenoidOnMask |=  (1u << n); }
void Machine::setSolenoidOff(uint8_t n)        { if (n < 32) solenoidOnMask &= ~(1u << n); }

void Machine::setDisplay(uint8_t idx, unsigned long value, uint8_t blankMask) {
    RPU_SetDisplay(idx, value, false, 1);
    RPU_SetDisplayBlank(idx, blankMask);
}

uint8_t Machine::numSounds()          const { return 0; }
void    Machine::playSound(uint8_t n) const { RPU_PlayNativeSound(n); }
void    Machine::stopAllSounds()      const {}
