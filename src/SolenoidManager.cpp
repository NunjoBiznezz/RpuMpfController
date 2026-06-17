#include "SolenoidManager.h"
#include "RPU.h"
#include "RPU_Internals.h"


SolenoidManager::EnabledStack SolenoidManager::Stack;
volatile uint8_t SolenoidManager::CurrentSolenoidByte = 0xFF;
volatile uint8_t SolenoidManager::RevertSolenoidBit = 0x00;
volatile uint8_t SolenoidManager::NumCyclesBeforeRevertingSolenoidByte = 0;
TimedStack<SolenoidManager::TimedEntry, SOL_TIMED_CAPACITY> SolenoidManager::TimedEntries;

#if (RPU_MPU_ARCHITECTURE >= 10)
unsigned short   SolenoidManager::ContinuousSolenoidBits = 0;
volatile uint8_t SolenoidManager::BoardLEDs = 0;
#endif

SolenoidManager& SolenoidManager::getInstance() {
    static SolenoidManager instance;
    return instance;
}

void SolenoidManager::push(uint8_t solenoidNumber, uint8_t numPushes, bool disableOverride) {
    if (!disableOverride && !Stack.isEnabled()) return;
    for (int count = 0; count < numPushes; count++) {
        if (!Stack.push(solenoidNumber)) return;
    }
}

void SolenoidManager::pushFront(uint8_t solenoidNumber, uint8_t numPushes) {
    if (!Stack.isEnabled()) return;
    for (int count = 0; count < numPushes; count++) {
        if (!Stack.pushFront(solenoidNumber)) return;
    }
}

bool SolenoidManager::pushTimed(uint8_t solenoidNumber, uint8_t numPushes,
                                 unsigned long whenToFire, bool disableOverride) {
    uint8_t slot = TimedEntries.findFreeSlot();
    if (slot >= TimedEntries.getSize()) return false;
    TimedEntries.entries[slot].inUse = true;
    TimedEntries.entries[slot].pushTime = whenToFire;
    TimedEntries.entries[slot].disableOverride = disableOverride;
    TimedEntries.entries[slot].solenoidNumber = solenoidNumber;
    TimedEntries.entries[slot].numPushes = numPushes;
    return true;
}

void SolenoidManager::updateTimed(unsigned long curTime) {
    for (int count = 0; count < TimedEntries.getSize(); count++) {
        if (TimedEntries.entries[count].inUse &&
            TimedEntries.entries[count].pushTime < curTime) {
            push(TimedEntries.entries[count].solenoidNumber,
                 TimedEntries.entries[count].numPushes,
                 TimedEntries.entries[count].disableOverride);
            TimedEntries.entries[count].inUse = false;
        }
    }
}

void SolenoidManager::enableStack() {
    Stack.setEnabled(true);
#if (RPU_MPU_ARCHITECTURE >= 10)
    RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x3C);
#endif
}

void SolenoidManager::disableStack() {
    Stack.setEnabled(false);
#if (RPU_MPU_ARCHITECTURE >= 10)
    RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x34);
#endif
}

#if (RPU_MPU_ARCHITECTURE < 10)

void SolenoidManager::setCoinLockout(bool lockoutOff, uint8_t solbit) {
    if (!lockoutOff) {
        CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
    } else {
        CurrentSolenoidByte = CurrentSolenoidByte | solbit;
    }
    RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}

void SolenoidManager::setDisableFlippers(bool disableFlippers, uint8_t solbit) {
    if (disableFlippers) {
        CurrentSolenoidByte = CurrentSolenoidByte | solbit;
    } else {
        CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
    }
    RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}

void SolenoidManager::setContinuousSolenoidBit(bool bitOn, uint8_t solbit) {
    if (bitOn) {
        CurrentSolenoidByte = CurrentSolenoidByte | solbit;
    } else {
        CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
    }
    RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}

bool SolenoidManager::fireContinuousSolenoid(uint8_t solBit, uint8_t numCyclesToFire) {
    if (NumCyclesBeforeRevertingSolenoidByte) return false;
    NumCyclesBeforeRevertingSolenoidByte = numCyclesToFire;
    RevertSolenoidBit = solBit;
    setContinuousSolenoidBit(false, solBit);
    return true;
}

uint8_t SolenoidManager::readContinuousSolenoids() {
    return RPU_DataRead(ADDRESS_U11_B);
}

#else // RPU_MPU_ARCHITECTURE >= 10

void SolenoidManager::setContinuousSolenoid(bool solOn, uint8_t solNum) {
    unsigned short oldCont = ContinuousSolenoidBits;
    if (solOn) {
        ContinuousSolenoidBits |= (1 << solNum);
    } else {
        ContinuousSolenoidBits &= ~(1 << solNum);
    }
    if (oldCont != ContinuousSolenoidBits) {
        uint8_t origPortA = RPU_DataRead(PIA_SOLENOID_PORT_A);
        uint8_t origPortB = RPU_DataRead(PIA_SOLENOID_PORT_B);
        if (origPortA != (ContinuousSolenoidBits & 0xFF)) {
            RPU_DataWrite(PIA_SOLENOID_PORT_A, (ContinuousSolenoidBits & 0xFF));
        }
        if (origPortB != (ContinuousSolenoidBits / 256)) {
            RPU_DataWrite(PIA_SOLENOID_PORT_B, (ContinuousSolenoidBits / 256));
        }
    }
}

void SolenoidManager::setCoinLockout(bool lockoutOn, uint8_t solNum) {
    setContinuousSolenoid(lockoutOn, solNum);
}

void SolenoidManager::setDisableFlippers(bool disableFlippers, uint8_t solbit) {
    (void) solbit;
    if (disableFlippers) {
        RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x34);
    } else {
        RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x3C);
    }
}

uint8_t SolenoidManager::readContinuousSolenoids() {
    return ContinuousSolenoidBits;
}

#endif

