#ifndef LAMP_MANAGER_H
#define LAMP_MANAGER_H

#include <stdint.h>


// Lamp management class for RPU
class LampManager {
public:
    static LampManager& getInstance();

    // Lamp state management
    void setLampState(int lampNum, uint8_t s_lampState, uint8_t s_lampDim = 0, int s_lampFlashPeriod = 0);
    void setLampAnimation(const uint8_t* lamps, unsigned count);
    uint8_t readLampState(int lampNum);
    uint8_t readLampDim(int lampNum);
    int readLampFlash(int lampNum);

    // Lamp control functions
    void applyFlashToLamps(unsigned long curTime);
    void flashAllLamps(unsigned long curTime);
    void turnOffAllLamps();

    // Dimming control
    void setDimDivisor(uint8_t level, uint8_t divisor);

    // Initialization
    void clearLampStates();

    // ISR-accessible functions for lamp multiplexing
    void multiplexLamps(int numberOfU10Interrupts);
    void multiplexAuxLamps(int numberOfU10Interrupts);

    // Direct access to lamp arrays for ISR
    static volatile uint8_t* getLampStates() { return LampStates; }
    static volatile uint8_t* getLampDim1() { return LampDim1; }
    static volatile uint8_t* getLampDim2() { return LampDim2; }
    static volatile uint8_t* getLampFlashPeriod() { return LampFlashPeriod; }
    static volatile uint8_t getDimDivisor1() { return DimDivisor1; }
    static volatile uint8_t getDimDivisor2() { return DimDivisor2; }

private:
    LampManager();
    ~LampManager() = default;
    LampManager(const LampManager&) = delete;
    LampManager& operator=(const LampManager&) = delete;

    // Lamp state arrays
    static volatile uint8_t LampStates[8];  // Assuming RPU_NUM_LAMP_BANKS = 8
    static volatile uint8_t LampDim1[8];
    static volatile uint8_t LampDim2[8];
    static volatile uint8_t LampFlashPeriod[64];  // Assuming RPU_MAX_LAMPS = 64

    // Dim divisor variables
    static volatile uint8_t DimDivisor1;
    static volatile uint8_t DimDivisor2;

    // Bit shift helper
    static const uint8_t BitShiftValues[8];
};


#endif // LAMP_MANAGER_H
