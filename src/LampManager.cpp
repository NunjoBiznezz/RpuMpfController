#include "LampManager.h"
#include "RPU.h"
#include "RPU_Internals.h"
#include <Arduino.h>

// Define the static members
volatile uint8_t LampManager::LampStates[8];
volatile uint8_t LampManager::LampDim1[8];
volatile uint8_t LampManager::LampDim2[8];
volatile uint8_t LampManager::LampFlashPeriod[64];
volatile uint8_t LampManager::DimDivisor1 = 2;
volatile uint8_t LampManager::DimDivisor2 = 2;

const uint8_t LampManager::BitShiftValues[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};


LampManager& LampManager::getInstance() {
    static LampManager instance;
    return instance;
}

LampManager::LampManager() {
    clearLampStates();
}

void LampManager::setLampState(int lampNum, uint8_t s_lampState, uint8_t s_lampDim, int s_lampFlashPeriod) {
    if (lampNum >= 64 || lampNum < 0) {  // Assuming RPU_MAX_LAMPS = 64
        return;
    }
    const uint8_t lampRow = lampNum % 8;
    const uint8_t lampCol = lampNum / 8;
    const uint8_t lampBit = BitShiftValues[lampRow];

    if (s_lampState) {
        int adjustedLampFlash = s_lampFlashPeriod / 50;

        if (s_lampFlashPeriod != 0 && adjustedLampFlash == 0) {
            adjustedLampFlash = 1;
        }
        if (adjustedLampFlash > 250) {
            adjustedLampFlash = 250;
        }

        // Only turn on the lamp if there's no flash, because if there's a flash
        // then the lamp will be turned on by the ApplyFlashToLamps function
        if (s_lampFlashPeriod == 0) {
            LampStates[lampCol] &= ~(lampBit);
        }
        LampFlashPeriod[lampNum] = adjustedLampFlash;
    } else {
        LampStates[lampCol] |= lampBit;
        LampFlashPeriod[lampNum] = 0;
    }

    if (s_lampDim & 0x01) {
        LampDim1[lampCol] |= lampBit;
    } else {
        LampDim1[lampCol] &= ~lampBit;
    }

    if (s_lampDim & 0x02) {
        LampDim2[lampCol] |= lampBit;
    } else {
        LampDim2[lampCol] &= ~lampBit;
    }
}

void LampManager::setLampAnimation(const uint8_t *lamps, unsigned count) {
    if (count > 0 && count <= 8) {
        for (unsigned index = 0; index < count; index++) {
            LampStates[index] = lamps[index];
            LampDim1[index] = 0;
            LampDim2[index] = 0;
        }
    }
}

uint8_t LampManager::readLampState(int lampNum) {
    if (lampNum >= 64 || lampNum < 0) {  // Assuming RPU_MAX_LAMPS = 64
        return 0x00;
    }
    const uint8_t lampStateByte = LampStates[lampNum / 8];
    return (lampStateByte & (0x01 << (lampNum % 8))) ? 0 : 1;
}

uint8_t LampManager::readLampDim(int lampNum) {
    if (lampNum >= 64 || lampNum < 0) {  // Assuming RPU_MAX_LAMPS = 64
        return 0x00;
    }
    uint8_t lampDim = 0;
    uint8_t lampDimByte = LampDim1[lampNum / 8];
    const uint16_t mask = (0x01 << (lampNum % 8));

    if (lampDimByte & mask) {
        lampDim |= 1;
    }

    lampDimByte = LampDim2[lampNum / 8];
    if (lampDimByte & mask) {
        lampDim |= 2;
    }

    return lampDim;
}

int LampManager::readLampFlash(int lampNum) {
    if (lampNum >= 64 || lampNum < 0) {  // Assuming RPU_MAX_LAMPS = 64
        return 0;
    }

    return LampFlashPeriod[lampNum] * 50;
}

void LampManager::applyFlashToLamps(unsigned long curTime) {
    int curLampByte = 0;
    uint8_t curLampBit = 0;
    int curLampNum = 0;

    for (curLampByte = 0; curLampByte < 8; curLampByte++) {  // Assuming RPU_NUM_LAMP_BANKS = 8
        curLampBit = 0x01;
        for (uint8_t curBit = 0; curBit < 8; curBit++) {
            if (LampFlashPeriod[curLampNum] != 0) {
                unsigned long adjustedLampFlash =
                      static_cast<unsigned long>(LampFlashPeriod[curLampNum]) * static_cast<unsigned long>(50);
                if ((curTime / adjustedLampFlash) % 2) {
                    LampStates[curLampByte] &= ~(curLampBit);
                } else {
                    LampStates[curLampByte] |= (curLampBit);
                }
            }

            curLampBit *= 2;
            curLampNum += 1;
        }
    }
}

void LampManager::flashAllLamps(unsigned long curTime) {
    for (int count = 0; count < 64; count++) {  // Assuming RPU_MAX_LAMPS = 64
        setLampState(count, 1, 0, 500);
    }

    applyFlashToLamps(curTime);
}

void LampManager::turnOffAllLamps() {
    for (int count = 0; count < 64; count++) {  // Assuming RPU_MAX_LAMPS = 64
        setLampState(count, 0, 0, 0);
    }
}

void LampManager::setDimDivisor(uint8_t level, uint8_t divisor) {
    if (level == 1) {
        DimDivisor1 = divisor;
    }
    if (level == 2) {
        DimDivisor2 = divisor;
    }
}

void LampManager::clearLampStates() {
    // Turn off all lamp states
    for (int lampBankCounter = 0; lampBankCounter < 8; lampBankCounter++) {  // Assuming RPU_NUM_LAMP_BANKS = 8
        LampStates[lampBankCounter] = 0xFF;
        LampDim1[lampBankCounter] = 0x00;
        LampDim2[lampBankCounter] = 0x00;
    }

    for (int lampFlashCount = 0; lampFlashCount < 64; lampFlashCount++) {  // Assuming RPU_MAX_LAMPS = 64
        LampFlashPeriod[lampFlashCount] = 0;
    }
}

void LampManager::multiplexLamps(int numberOfU10Interrupts) {
    // This function contains the lamp multiplexing logic from the ISR
    // It needs access to RPU_DataWrite, RPU_DataRead functions

    for (int lampByteCount = 0; lampByteCount < 8; lampByteCount++) {
        for (uint8_t nibbleCount = 0; nibbleCount < 2; nibbleCount++) {
            // We skip iteration number 16 because the last position is to park the lamps
            if (lampByteCount == (7) && nibbleCount) {
                continue;
            }

            uint8_t lampData = 0xF0 + (lampByteCount * 2) + nibbleCount;

            // Latch address & strobe
            RPU_DataWrite(ADDRESS_U10_A, 0xFF);
            RPU_DataWrite(ADDRESS_U10_A, lampData);
            RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x38);
            RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x30);

            // Use the inhibit lines to set the actual data to the lamp SCRs
            // (here, we don't care about the lower nibble because the address was already latched)
            uint8_t nibbleOffset = (nibbleCount) ? 1 : 16;
            uint8_t lampOutput = (LampStates[lampByteCount] * nibbleOffset);

            // Every other time through the cycle, we OR in the dim variable
            // in order to dim those lights
            if (numberOfU10Interrupts % DimDivisor1) {
                lampOutput |= (LampDim1[lampByteCount] * nibbleOffset);
            }
            if (numberOfU10Interrupts % DimDivisor2) {
                lampOutput |= (LampDim2[lampByteCount] * nibbleOffset);
            }

            RPU_DataWrite(ADDRESS_U10_A, lampOutput | 0x0F);
        } // end loop on nibble
    } // end loop on lamp bytes
}

void LampManager::multiplexAuxLamps(int numberOfU10Interrupts) {
    // Similar to multiplexLamps but for auxiliary lamps

    // Latch 0xFF separately without interrupt clear
    // to park 0xFF in main lamp board
    RPU_DataWrite(ADDRESS_U10_A, 0xFF);
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
    RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

    // For the first four bits of lamps, we're going to look at LampStates[7] again
    // and use those top 4 bits that we didn't use before. Then we're going
    // to move on with bytes 8, 9, and 10 for the remaining 24 bits of data
    uint8_t auxBankNum = 0;
    for (int lampByteCount = 7; lampByteCount < 8; lampByteCount++) {  // Assuming RPU_NUM_LAMP_BANKS = 8
        for (uint8_t nibbleCount = 0; nibbleCount < 2; nibbleCount++) {
            if (lampByteCount == 7) {
                nibbleCount = 1; // skip the first nibble of byte 7 because it belongs to primary lamps
            }
            uint8_t nibbleOffset = (nibbleCount) ? 1 : 16;
            uint8_t lampOutput = (LampStates[lampByteCount] * nibbleOffset);
            // Every other time through the cycle, we OR in the dim variable
            // in order to dim those lights
            if (numberOfU10Interrupts % DimDivisor1) {
                lampOutput |= (LampDim1[lampByteCount] * nibbleOffset);
            }
            if (numberOfU10Interrupts % DimDivisor2) {
                lampOutput |= (LampDim2[lampByteCount] * nibbleOffset);
            }

            // The data will be in the upper nibble, but we need the bank count in the lower
            lampOutput &= 0xF0;
            lampOutput += auxBankNum;

            RPU_DataWrite(ADDRESS_U10_A, lampOutput | 0xF0);
            RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
            RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
            RPU_DataWrite(ADDRESS_U10_A, lampOutput);

            auxBankNum += 1;
        }
    }
}

