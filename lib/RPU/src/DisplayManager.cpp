#include "DisplayManager.h"
#include "RPU_config.h"
#include "RPU_Internals.h"
#include <Arduino.h>

volatile uint8_t RPU::DisplayManager::DisplayDigits[5][RPU_OS_NUM_DIGITS];
volatile uint8_t RPU::DisplayManager::DisplayDigitEnable[5];
volatile bool    RPU::DisplayManager::DisplayOffCycle = false;
volatile uint8_t RPU::DisplayManager::CurrentDisplayDigit = 0;

#if (RPU_MPU_ARCHITECTURE >= 10)
volatile uint8_t RPU::DisplayManager::DisplayCreditDigits[2];
volatile uint8_t RPU::DisplayManager::DisplayCreditDigitEnable = 0;
volatile uint8_t RPU::DisplayManager::DisplayBIPDigits[2];
volatile uint8_t RPU::DisplayManager::DisplayBIPDigitEnable = 0;
#endif

#if (RPU_MPU_ARCHITECTURE >= 13)
volatile uint8_t RPU::DisplayManager::DisplayCommas = 0;
#endif

#if (RPU_MPU_ARCHITECTURE == 15)
volatile uint8_t RPU::DisplayManager::DisplayText[2][RPU_OS_NUM_DIGITS];

const uint16_t RPU::DisplayManager::SevenSegmentNumbers[10] = {
   0x3F, /* 0 */
   0x06, /* 1 */
   0x5B, /* 2 */
   0x4F, /* 3 */
   0x66, /* 4 */
   0x6D, /* 5 */
   0x7D, /* 6 */
   0x07, /* 7 */
   0x7F, /* 8 */
   0x6F  /* 9 */
};

const uint16_t RPU::DisplayManager::FourteenSegmentASCII[96] = {
   0x0000, /*   */
   0x0006, /* ! */
   0x0102, /* " */
   0x154E, /* # */
   0x156D, /* $ */
   0x3FE4, /* % */
   0x09D9, /* & */
   0x0100, /* ' */
   0x0A00, /* ( */
   0x2080, /* ) */
   0x3FC0, /* * */
   0x1540, /* + */
   0x2000, /* , */
   0x0440, /* - */
   0x0000, /* . */
   0x2200, /* / */
   0x223F, /* 0 */
   0x0206, /* 1 */
   0x045B, /* 2 */
   0x040F, /* 3 */
   0x0466, /* 4 */
   0x0869, /* 5 */
   0x047D, /* 6 */
   0x0007, /* 7 */
   0x047F, /* 8 */
   0x046F, /* 9 */
   0x1100, /* : */
   0x2100, /* ; */
   0x0A40, /* < */
   0x0448, /* = */
   0x2480, /* > */
   0x1403, /* ? */
   0x053B, /* @ */
   0x0477, /* A */
   0x150F, /* B */
   0x0039, /* C */
   0x110F, /* D */
   0x0079, /* E */
   0x0071, /* F */
   0x043D, /* G */
   0x0476, /* H */
   0x1109, /* I */
   0x001E, /* J */
   0x0A70, /* K */
   0x0038, /* L */
   0x02B6, /* M */
   0x08B6, /* N */
   0x003F, /* O */
   0x0473, /* P */
   0x083F, /* Q */
   0x0C73, /* R */
   0x046D, /* S */
   0x1101, /* T */
   0x003E, /* U */
   0x2230, /* V */
   0x2836, /* W */
   0x2A80, /* X */
   0x046E, /* Y */
   0x2209, /* Z */
   0x0039, /* [ */
   0x0880, /* \ */
   0x000F, /* ] */
   0x2800, /* ^ */
   0x0008, /* _ */
   0x0080, /* ` */
   0x1058, /* a */
   0x0878, /* b */
   0x0458, /* c */
   0x240E, /* d */
   0x2058, /* e */
   0x1640, /* f */
   0x060E, /* g */
   0x1070, /* h */
   0x1000, /* i */
   0x2110, /* j */
   0x1B00, /* k */
   0x0030, /* l */
   0x1454, /* m */
   0x1050, /* n */
   0x045C, /* o */
   0x00F0, /* p */
   0x0606, /* q */
   0x0050, /* r */
   0x0C08, /* s */
   0x0078, /* t */
   0x001C, /* u */
   0x2010, /* v */
   0x2814, /* w */
   0x2A80, /* x */
   0x050E, /* y */
   0x2048, /* z */
   0x20C9, /* { */
   0x1100, /* | */
   0x0E09, /* } */
   0x2640, /* ~ */
   0x0000  /*   */
};
#endif

namespace RPU {

DisplayManager& DisplayManager::getInstance() {
    static DisplayManager instance;
    return instance;
}

DisplayManager::DisplayManager() {
    for (int d = 0; d < 5; d++) {
        for (int i = 0; i < RPU_OS_NUM_DIGITS; i++) {
            DisplayDigits[d][i] = 0;
        }
        DisplayDigitEnable[d] = 0;
    }
#if (RPU_MPU_ARCHITECTURE >= 10)
    DisplayCreditDigits[0] = 0;
    DisplayCreditDigits[1] = 0;
    DisplayCreditDigitEnable = 0;
    DisplayBIPDigits[0] = 0;
    DisplayBIPDigits[1] = 0;
    DisplayBIPDigitEnable = 0;
#endif
#if (RPU_MPU_ARCHITECTURE == 15)
    for (int d = 0; d < 2; d++) {
        for (int i = 0; i < RPU_OS_NUM_DIGITS; i++) {
            DisplayText[d][i] = 0;
        }
    }
#endif
}

#if (RPU_MPU_ARCHITECTURE == 15)
uint8_t DisplayManager::setDisplay(int displayNumber, unsigned long value,
                                    bool blankByMagnitude, uint8_t minDigits,
                                    bool showCommasByMagnitude) {
    (void) showCommasByMagnitude;
    if (displayNumber < 0 || displayNumber > 3) {
        return 0;
    }

    uint8_t blank = 0x00;

    for (int count = 0; count < RPU_OS_NUM_DIGITS; count++) {
        blank = blank * 2;
        if (value != 0 || count < minDigits) {
            blank |= 1;
            if (displayNumber / 2) {
                DisplayDigits[displayNumber][(RPU_OS_NUM_DIGITS - 1) - count] = SevenSegmentNumbers[value % 10];
            } else {
                DisplayText[displayNumber][(RPU_OS_NUM_DIGITS - 1) - count] = (value % 10) + 16;
            }
        } else {
            if (displayNumber / 2) {
                DisplayDigits[displayNumber][(RPU_OS_NUM_DIGITS - 1) - count] = 0;
            } else {
                DisplayText[displayNumber][(RPU_OS_NUM_DIGITS - 1) - count] = 0;
            }
        }
        value /= 10;
    }

    if (blankByMagnitude) {
        DisplayDigitEnable[displayNumber] = blank;
    }

    return blank;
}
#else
uint8_t DisplayManager::setDisplay(int displayNumber, unsigned long value,
                                    bool blankByMagnitude, uint8_t minDigits,
                                    bool showCommasByMagnitude) {
    if (displayNumber < 0 || displayNumber > 4) {
        return 0;
    }

    uint8_t blank = 0x00;
#if (RPU_MPU_ARCHITECTURE >= 13)
    uint8_t commaBit = 0x01 << (2 * displayNumber);
#endif

    for (int count = 0; count < RPU_OS_NUM_DIGITS; count++) {
        blank = blank * 2;
        if (value != 0 || count < minDigits) {
            blank |= 1;
        }

#if (RPU_MPU_ARCHITECTURE >= 13)
        if (showCommasByMagnitude) {
            if (value) {
                if (count == 3) {
                    DisplayCommas |= commaBit;
                }
                if (count == 6) {
                    DisplayCommas |= (commaBit * 2);
                }
            } else {
                if (count == 3) {
                    DisplayCommas &= ~(commaBit);
                }
                if (count == 6) {
                    DisplayCommas &= ~(commaBit * 2);
                }
            }
        }
#else
        (void) showCommasByMagnitude;
#endif
        DisplayDigits[displayNumber][(RPU_OS_NUM_DIGITS - 1) - count] = value % 10;
        value /= 10;
    }

    if (blankByMagnitude) {
        DisplayDigitEnable[displayNumber] = blank;
    }

    return blank;
}
#endif

void DisplayManager::setDisplayBlank(int displayNumber, uint8_t bitMask) {
    if (displayNumber < 0 || displayNumber > 4) {
        return;
    }
    DisplayDigitEnable[displayNumber] = bitMask;
}

uint8_t DisplayManager::getDisplayBlank(int displayNumber) const {
    if (displayNumber < 0 || displayNumber > 4) {
        return 0;
    }
    return DisplayDigitEnable[displayNumber];
}

void DisplayManager::setDisplayFlash(int displayNumber, unsigned long value,
                                      unsigned long curTime, int period, uint8_t minDigits) {
    if (period) {
        if ((curTime / period) % 2) {
            setDisplay(displayNumber, value, true, minDigits);
        } else {
            setDisplayBlank(displayNumber, 0);
        }
    }
}

void DisplayManager::setDisplayFlashCredits(unsigned long curTime, int period) {
    if (period) {
        if ((curTime / period) % 2) {
            DisplayDigitEnable[4] |= 0x06;
        } else {
            DisplayDigitEnable[4] &= 0x39;
        }
    }
}

void DisplayManager::cycleAllDisplays(unsigned long curTime, uint8_t digitNum) {
    int displayDigit = (curTime / 250) % 10;
    unsigned long value;
#if (RPU_OS_NUM_DIGITS == 7)
    value = displayDigit * 1111111;
#else
    value = displayDigit * 111111;
#endif

    uint8_t displayNumToShow = 0;
    uint8_t displayBlank = RPU_OS_ALL_DIGITS_MASK;

    if (digitNum != 0) {
#if (RPU_OS_NUM_DIGITS == 7)
        displayNumToShow = (digitNum - 1) / 7;
        displayBlank = (0x40) >> ((digitNum - 1) % 7);

#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
        if (displayNumToShow == 4) {
            displayBlank = (0x20) >> ((digitNum - 1) % 6);
        }
#endif

#else
        displayNumToShow = (digitNum - 1) / 6;
        displayBlank = (0x20) >> ((digitNum - 1) % 6);
#endif
    }

    for (int count = 0; count < 5; count++) {
        if (digitNum) {
            setDisplay(count, value);
            if (count == displayNumToShow) {
                setDisplayBlank(count, displayBlank);
            } else {
                setDisplayBlank(count, 0);
            }
        } else {
            setDisplay(count, value, false);
        }
    }
}

void DisplayManager::setDisplayMatch(int value, bool displayOn, bool showBothDigits) {
    setDisplayBallInPlay(value, displayOn, showBothDigits);
}

#if (RPU_MPU_ARCHITECTURE < 10)
void DisplayManager::setDisplayCredits(int value, bool displayOn, bool showBothDigits) {
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
    DisplayDigits[4][2] = (value % 100) / 10;
    DisplayDigits[4][3] = (value % 10);
#else
    DisplayDigits[4][1] = (value % 100) / 10;
    DisplayDigits[4][2] = (value % 10);
#endif
    uint8_t enableMask = DisplayDigitEnable[4] & RPU_OS_MASK_SHIFT_1;

    if (displayOn) {
        if (value > 9 || showBothDigits) {
            enableMask |= RPU_OS_MASK_SHIFT_2;
        } else {
            enableMask |= 0x04;
        }
    }

    DisplayDigitEnable[4] = enableMask;
}

void DisplayManager::setDisplayBallInPlay(int value, bool displayOn, bool showBothDigits) {
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
    DisplayDigits[4][5] = (value % 100) / 10;
    DisplayDigits[4][6] = (value % 10);
#else
    DisplayDigits[4][4] = (value % 100) / 10;
    DisplayDigits[4][5] = (value % 10);
#endif
    uint8_t enableMask = DisplayDigitEnable[4] & RPU_OS_MASK_SHIFT_2;

    if (displayOn) {
        if (value > 9 || showBothDigits) {
            enableMask |= RPU_OS_MASK_SHIFT_1;
        } else {
            enableMask |= 0x20;
        }
    }

    DisplayDigitEnable[4] = enableMask;
}

#elif (RPU_MPU_ARCHITECTURE < 15)
void DisplayManager::setDisplayCredits(int value, bool displayOn, bool showBothDigits) {
    uint8_t blank = 0x02;
    value = value % 100;
    if (value >= 10) {
        DisplayCreditDigits[0] = value / 10;
        blank |= 1;
    } else {
        DisplayCreditDigits[0] = 0;
        if (showBothDigits) {
            blank |= 1;
        }
    }
    DisplayCreditDigits[1] = value % 10;
    DisplayCreditDigitEnable = displayOn ? blank : 0;
}

void DisplayManager::setDisplayBallInPlay(int value, bool displayOn, bool showBothDigits) {
    uint8_t blank = 0x02;
    value = value % 100;
    if (value >= 10) {
        DisplayBIPDigits[0] = value / 10;
        blank |= 1;
    } else {
        DisplayBIPDigits[0] = 0;
        if (showBothDigits) {
            blank |= 1;
        }
    }
    DisplayBIPDigits[1] = value % 10;
    DisplayBIPDigitEnable = displayOn ? blank : 0;
}

#else // RPU_MPU_ARCHITECTURE == 15
void DisplayManager::setDisplayCredits(int value, bool displayOn, bool showBothDigits) {
    uint8_t blank = 0x02;
    value = value % 100;
    if (value >= 10) {
        DisplayCreditDigits[0] = SevenSegmentNumbers[value / 10];
        blank |= 1;
    } else {
        DisplayCreditDigits[0] = SevenSegmentNumbers[0];
        if (showBothDigits) {
            blank |= 1;
        }
    }
    DisplayCreditDigits[1] = SevenSegmentNumbers[value % 10];
    DisplayCreditDigitEnable = displayOn ? blank : 0;
}

void DisplayManager::setDisplayBallInPlay(int value, bool displayOn, bool showBothDigits) {
    uint8_t blank = 0x02;
    value = value % 100;
    if (value >= 10) {
        DisplayBIPDigits[0] = SevenSegmentNumbers[value / 10];
        blank |= 1;
    } else {
        DisplayBIPDigits[0] = SevenSegmentNumbers[0];
        if (showBothDigits) {
            blank |= 1;
        }
    }
    DisplayBIPDigits[1] = SevenSegmentNumbers[value % 10];
    DisplayBIPDigitEnable = displayOn ? blank : 0;
}
#endif

#if defined(RPU_OS_ADJUSTABLE_DISPLAY_INTERRUPT)
void DisplayManager::setDisplayRefreshConstant(int intervalConstant) {
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = intervalConstant;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS12) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);
    sei();
}
#endif

#if (RPU_MPU_ARCHITECTURE == 15)
uint8_t DisplayManager::setDisplayText(int displayNumber, char* text, bool blankByLength) {
    if (displayNumber > 1 || displayNumber < 0) {
        return 0;
    }
    uint8_t stringLength = 0xff;
    bool writeSpace = false;
    uint8_t blank = 0;
    uint8_t placeMask = 0x01;

    for (stringLength = 0; stringLength < RPU_OS_NUM_DIGITS; stringLength++) {
        if (text[stringLength] == 0) {
            writeSpace = true;
        }
        if (!writeSpace) {
            DisplayText[displayNumber][stringLength] = (uint8_t) text[stringLength] - 0x20;
        } else {
            DisplayText[displayNumber][stringLength] = 0;
        }

        if (DisplayText[displayNumber][stringLength]) {
            blank |= placeMask;
        }
        placeMask *= 2;
    }

    if (blankByLength) {
        DisplayDigitEnable[displayNumber] = blank;
    }

    return stringLength;
}
#endif

} // namespace RPU
