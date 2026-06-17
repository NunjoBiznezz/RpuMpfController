#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdint.h>
#include "RPU_config.h"

namespace RPU {

class DisplayManager {
public:
    static DisplayManager& getInstance();

    uint8_t setDisplay(int displayNumber, unsigned long value,
                       bool blankByMagnitude = false, uint8_t minDigits = 2,
                       bool showCommasByMagnitude = false);
    void setDisplayBlank(int displayNumber, uint8_t bitMask);
    uint8_t getDisplayBlank(int displayNumber) const;
    void setDisplayFlash(int displayNumber, unsigned long value,
                         unsigned long curTime, int period = 500, uint8_t minDigits = 2);
    void setDisplayFlashCredits(unsigned long curTime, int period = 100);
    void cycleAllDisplays(unsigned long curTime, uint8_t digitNum = 0);
    void setDisplayCredits(int value, bool displayOn = true, bool showBothDigits = true);
    void setDisplayBallInPlay(int value, bool displayOn = true, bool showBothDigits = true);
    void setDisplayMatch(int value, bool displayOn = true, bool showBothDigits = true);

#if defined(RPU_OS_ADJUSTABLE_DISPLAY_INTERRUPT)
    void setDisplayRefreshConstant(int intervalConstant);
#endif

#if (RPU_MPU_ARCHITECTURE == 15)
    uint8_t setDisplayText(int displayNumber, char* text, bool blankByLength = true);
#endif

    // ISR-accessible state
    static volatile uint8_t* getDisplayDigits(int displayNumber) { return DisplayDigits[displayNumber]; }
    static volatile uint8_t  getDisplayDigitEnable(int displayNumber) { return DisplayDigitEnable[displayNumber]; }
    static volatile bool      getDisplayOffCycle() { return DisplayOffCycle; }
    static void               setDisplayOffCycle(bool offCycle) { DisplayOffCycle = offCycle; }
    static volatile uint8_t   getCurrentDisplayDigit() { return CurrentDisplayDigit; }
    static void               setCurrentDisplayDigit(uint8_t digit) { CurrentDisplayDigit = digit; }

#if (RPU_MPU_ARCHITECTURE >= 10)
    static volatile uint8_t* getDisplayCreditDigits()      { return DisplayCreditDigits; }
    static volatile uint8_t  getDisplayCreditDigitEnable() { return DisplayCreditDigitEnable; }
    static volatile uint8_t* getDisplayBIPDigits()         { return DisplayBIPDigits; }
    static volatile uint8_t  getDisplayBIPDigitEnable()    { return DisplayBIPDigitEnable; }
#endif

#if (RPU_MPU_ARCHITECTURE >= 13)
    static volatile uint8_t getDisplayCommas() { return DisplayCommas; }
#endif

#if (RPU_MPU_ARCHITECTURE == 15)
    static volatile uint8_t* getDisplayText(int displayNumber) { return DisplayText[displayNumber]; }
#endif

private:
    DisplayManager();
    ~DisplayManager() = default;
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;

    static volatile uint8_t DisplayDigits[5][RPU_OS_NUM_DIGITS];
    static volatile uint8_t DisplayDigitEnable[5];
    static volatile bool    DisplayOffCycle;
    static volatile uint8_t CurrentDisplayDigit;

#if (RPU_MPU_ARCHITECTURE >= 10)
    static volatile uint8_t DisplayCreditDigits[2];
    static volatile uint8_t DisplayCreditDigitEnable;
    static volatile uint8_t DisplayBIPDigits[2];
    static volatile uint8_t DisplayBIPDigitEnable;
#endif

#if (RPU_MPU_ARCHITECTURE >= 13)
    static volatile uint8_t DisplayCommas;
#endif

#if (RPU_MPU_ARCHITECTURE == 15)
    static volatile uint8_t DisplayText[2][RPU_OS_NUM_DIGITS];
    static const uint16_t   SevenSegmentNumbers[10];
    static const uint16_t   FourteenSegmentASCII[96];
#endif
};

} // namespace RPU

#endif // DISPLAY_MANAGER_H
