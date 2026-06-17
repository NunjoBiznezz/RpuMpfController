#ifndef SOLENOID_MANAGER_H
#define SOLENOID_MANAGER_H

#include <stdint.h>
#include "RPU_config.h"
#include "CircularStack.h"
#include "TimedStack.h"

namespace RPU {

#if (RPU_OS_HARDWARE_REV > 2)
constexpr uint8_t SOL_STACK_CAPACITY = 150;
#else
constexpr uint8_t SOL_STACK_CAPACITY = 60;
#endif
constexpr uint8_t SOL_TIMED_CAPACITY = 30;

class SolenoidManager {
public:
    static SolenoidManager& getInstance();

    void push(uint8_t solenoidNumber, uint8_t numPushes, bool disableOverride);
    void pushFront(uint8_t solenoidNumber, uint8_t numPushes);
    bool pushTimed(uint8_t solenoidNumber, uint8_t numPushes,
                   unsigned long whenToFire, bool disableOverride);
    void updateTimed(unsigned long curTime);
    void enableStack();
    void disableStack();

#if (RPU_MPU_ARCHITECTURE < 10)
    void    setCoinLockout(bool lockoutOff, uint8_t solbit);
    void    setDisableFlippers(bool disableFlippers, uint8_t solbit);
    void    setContinuousSolenoidBit(bool bitOn, uint8_t solbit);
    bool    fireContinuousSolenoid(uint8_t solBit, uint8_t numCyclesToFire);
    uint8_t readContinuousSolenoids();
#else
    void    setContinuousSolenoid(bool solOn, uint8_t solNum);
    void    setCoinLockout(bool lockoutOn, uint8_t solNum);
    void    setDisableFlippers(bool disableFlippers, uint8_t solbit);
    uint8_t readContinuousSolenoids();
#endif

    // ISR-accessible static accessors
    static uint8_t stackPull()      { return Stack.pull(); }
    static bool    stackIsEmpty()   { return Stack.isEmpty(); }
    static bool    stackIsEnabled() { return Stack.isEnabled(); }

    static volatile uint8_t getCurrentSolenoidByte()               { return CurrentSolenoidByte; }
    static void             setCurrentSolenoidByte(uint8_t val)    { CurrentSolenoidByte = val; }
    static volatile uint8_t getRevertSolenoidBit()                 { return RevertSolenoidBit; }
    static void             setRevertSolenoidBit(uint8_t val)      { RevertSolenoidBit = val; }
    static volatile uint8_t getNumCyclesBeforeReverting()          { return NumCyclesBeforeRevertingSolenoidByte; }
    static void             setNumCyclesBeforeReverting(uint8_t v) { NumCyclesBeforeRevertingSolenoidByte = v; }
    static void             decrementNumCycles() {
        if (NumCyclesBeforeRevertingSolenoidByte) NumCyclesBeforeRevertingSolenoidByte--;
    }

#if (RPU_MPU_ARCHITECTURE >= 10)
    static unsigned short   getContinuousSolenoidBits()            { return ContinuousSolenoidBits; }
    static volatile uint8_t getBoardLEDs()                         { return BoardLEDs; }
    static void             setBoardLEDs(uint8_t val)              { BoardLEDs = val; }
#endif

private:
    SolenoidManager() = default;
    ~SolenoidManager() = default;
    SolenoidManager(const SolenoidManager&) = delete;
    SolenoidManager& operator=(const SolenoidManager&) = delete;

    struct EnabledStack : public CircularStack<uint8_t, SOL_STACK_CAPACITY, 0xFF> {
        bool isEnabled() const { return enabled_; }
        void setEnabled(bool v) { enabled_ = v; }
    private:
        bool enabled_ = true;
    };

    struct TimedEntry {
        uint8_t inUse;
        unsigned long pushTime;
        uint8_t solenoidNumber;
        uint8_t numPushes;
        uint8_t disableOverride;
    };

    static EnabledStack Stack;
    static volatile uint8_t CurrentSolenoidByte;
    static volatile uint8_t RevertSolenoidBit;
    static volatile uint8_t NumCyclesBeforeRevertingSolenoidByte;
    static TimedStack<TimedEntry, SOL_TIMED_CAPACITY> TimedEntries;

#if (RPU_MPU_ARCHITECTURE >= 10)
    static unsigned short   ContinuousSolenoidBits;
    static volatile uint8_t BoardLEDs;
#endif
};

} // namespace RPU

#endif // SOLENOID_MANAGER_H
