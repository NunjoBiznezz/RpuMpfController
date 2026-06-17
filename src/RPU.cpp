/**************************************************************************
 *     This file is part of the RPU for Arduino Project.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 3/31/2023

    RPU is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPU is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#include "RPU_config.h"
#include "RPU.h"
#include "RPU_Internals.h"
#include "LampManager.h"
#include "DisplayManager.h"
#include "SwitchManager.h"
#include "SolenoidManager.h"
#include "CircularStack.h"
#include "TimedStack.h"
#include <Arduino.h>
#include <EEPROM.h>


#ifdef RPU_OS_USE_DIP_SWITCHES
  void ReadDipSwitches();
#endif

   /******************************************************
    *   Defines and library variables
    */
#if !defined(RPU_MPU_BUILD_FOR_6800) || (RPU_MPU_BUILD_FOR_6800 == 1)
constexpr bool UsesM6800Processor = true;
#if (RPU_MPU_ARCHITECTURE > 11) && (RPU_OS_HARDWARE_REV < 102)
#error \
    "Architecture > 11 doesn't make sense with RPU_MPU_BUILD_FOR_6800=1. Set RPU_MPU_BUILD_FOR_6800 to 0 in RPU_Config.h or choose a different RPU_MPU_ARCHITECTURE"
#endif
#else
constexpr bool UsesM6800Processor = false;
#endif

#if (RPU_MPU_ARCHITECTURE < 10)

#ifdef RPU_USE_EXTENDED_SWITCHES_ON_PB4
#define NUM_SWITCH_BYTES 6
#define NUM_SWITCH_BYTES_ON_U10_PORT_A 5
#define MAX_NUM_SWITCHES 48
#define DEFAULT_SOLENOID_STATE 0x8F
#define ST5_CONTINUOUS_SOLENOID_BIT 0x10
#elif defined(RPU_USE_EXTENDED_SWITCHES_ON_PB7)
#define NUM_SWITCH_BYTES 6
#define NUM_SWITCH_BYTES_ON_U10_PORT_A 5
#define MAX_NUM_SWITCHES 48
#define DEFAULT_SOLENOID_STATE 0x1F
#define ST5_CONTINUOUS_SOLENOID_BIT 0x80
#else
#define RPU_NUM_SOLENOIDS 15
#define NUM_SWITCH_BYTES 5
#define NUM_SWITCH_BYTES_ON_U10_PORT_A 5
#define MAX_NUM_SWITCHES 40
#define DEFAULT_SOLENOID_STATE 0x9F
#endif

#if !defined(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS) || !defined(RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS)
#error "Must define RPU_OS_SWITCH_DELAY_IN_MICROSECONDS and RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS in RPU_Config.h"
#endif

#elif (RPU_MPU_ARCHITECTURE >= 10)
#define RPU_NUM_SOLENOIDS 22
#define NUM_SWITCH_BYTES 8
#define MAX_NUM_SWITCHES 64
#ifndef INTERRUPT_OCR1A_COUNTER
#define INTERRUPT_OCR1A_COUNTER 16574
#endif

#endif

   /******************************************************
    *   Display Configuration Functions
   ******************************************************/

   /******************************************************
    *   Display Configuration Functions
    *
    *   This function sets the display parameters for a specified display.
    *
    *   @param displayNumber The ID or index of the display to configure.
    *   @param value The numerical value to display.
    *   @param blankByMagnitude If true, the display will blank based on its magnitude.
    *   @param minDigits Minimum number of digits to display.
    *   @param showCommasByMagnitude If true, commas will be shown in the display based on its magnitude.
    *   @return A status code indicating success or failure of the operation.
    ******************************************************/
   uint8_t RPU_SetDisplay(int displayNumber, unsigned long value, bool blankByMagnitude,
                          uint8_t minDigits, bool showCommasByMagnitude) {
    return DisplayManager::getInstance().setDisplay(displayNumber, value, blankByMagnitude,
                                                    minDigits, showCommasByMagnitude);
}

void RPU_SetDisplayBlank(int displayNumber, uint8_t bitMask) {
    DisplayManager::getInstance().setDisplayBlank(displayNumber, bitMask);
}

uint8_t RPU_GetDisplayBlank(int displayNumber) {
    return DisplayManager::getInstance().getDisplayBlank(displayNumber);
}

void RPU_SetDisplayCredits(int value, bool displayOn, bool showBothDigits) {
    DisplayManager::getInstance().setDisplayCredits(value, displayOn, showBothDigits);
}

void RPU_SetDisplayBallInPlay(int value, bool displayOn, bool showBothDigits) {
    DisplayManager::getInstance().setDisplayBallInPlay(value, displayOn, showBothDigits);
}

void RPU_SetDisplayMatch(int value, bool displayOn, bool showBothDigits) {
    DisplayManager::getInstance().setDisplayMatch(value, displayOn, showBothDigits);
}

void RPU_SetDisplayFlash(int displayNumber, unsigned long value, unsigned long curTime,
                         int period, uint8_t minDigits) {
    DisplayManager::getInstance().setDisplayFlash(displayNumber, value, curTime, period, minDigits);
}

void RPU_SetDisplayFlashCredits(unsigned long curTime, int period) {
    DisplayManager::getInstance().setDisplayFlashCredits(curTime, period);
}

void RPU_CycleAllDisplays(unsigned long curTime, uint8_t digitNum) {
    DisplayManager::getInstance().cycleAllDisplays(curTime, digitNum);
}

#if defined(RPU_OS_ADJUSTABLE_DISPLAY_INTERRUPT)
void RPU_SetDisplayRefreshConstant(int intervalConstant) {
    DisplayManager::getInstance().setDisplayRefreshConstant(intervalConstant);
}
#endif

#if (RPU_MPU_ARCHITECTURE == 15)
uint8_t RPU_SetDisplayText(int displayNumber, char* text, bool blankByLength) {
    return DisplayManager::getInstance().setDisplayText(displayNumber, text, blankByLength);
}
#endif





   // The WTYPE1 and WTYPE2 sound cards can only play one sound at a time,
   // so these structures allow the app to send in as many calls as they
   // want, but with a priority and requested amount of time to let
   // it play. This could be ported over to other architectures
   // like S&T or -51, etc., but right now I've only implemented it for
   // MPU Architecture > 9
#if (RPU_MPU_ARCHITECTURE >= 10)

#define SOUND_STACK_SIZE 64
#define SOUND_STACK_EMPTY 0x0000
   static CircularStack<unsigned short, SOUND_STACK_SIZE, SOUND_STACK_EMPTY> SoundStack;

#define TIMED_SOUND_STACK_SIZE 20
   struct TimedSoundEntry {
      uint8_t inUse;
      unsigned long pushTime;
      unsigned short soundNumber;
      uint8_t numPushes;
   };
   static TimedStack<TimedSoundEntry, TIMED_SOUND_STACK_SIZE> TimedSoundStack;
#endif

#if (RPU_OS_HARDWARE_REV == 1) or (RPU_OS_HARDWARE_REV == 2)

#if defined(__AVR_ATmega2560__)
#error "ATMega requires RPU_OS_HARDWARE_REV of 3, check RPU_Config.h and adjust settings"
#endif

   void RPU_DataWrite(int address, uint8_t data) {
      // Set data pins to output
      // Make pins 5-7 output (and pin 3 for R/W)
      DDRD = DDRD | 0xE8;
      // Make pins 8-12 output
      DDRB = DDRB | 0x1F;

      // Set R/W to LOW
      PORTD = (PORTD & 0xF7);

      // Put data on pins
      // Put lower three bits on 5-7
      PORTD = (PORTD & 0x1F) | ((data & 0x07) << 5);
      // Put upper five bits on 8-12
      PORTB = (PORTB & 0xE0) | (data >> 3);

      // Set up address lines
      PORTC = (PORTC & 0xE0) | address;

      // Wait for a falling edge of the clock
      while ((PIND & 0x10));

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTC = PORTC | 0x20;

      // Wait while clock is low
      while (!(PIND & 0x10));

      // Wait while clock is high
      while ((PIND & 0x10));

      // Wait while clock is low
      while (!(PIND & 0x10));

      // Set VMA OFF
      PORTC = PORTC & 0xDF;

      // Unset address lines
      PORTC = PORTC & 0xE0;

      // Set R/W back to HIGH
      PORTD = (PORTD | 0x08);

      // Set data pins to input
      // Make pins 5-7 input
      DDRD = DDRD & 0x1F;
      // Make pins 8-12 input
      DDRB = DDRB & 0xE0;
   }

   uint8_t RPU_DataRead(int address) {
      // Set data pins to input
      // Make pins 5-7 input
      DDRD = DDRD & 0x1F;
      // Make pins 8-12 input
      DDRB = DDRB & 0xE0;

      // Set R/W to HIGH
      DDRD = DDRD | 0x08;
      PORTD = (PORTD | 0x08);

      // Set up address lines
      PORTC = (PORTC & 0xE0) | address;

      // Wait for a falling edge of the clock
      while ((PIND & 0x10));

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTC = PORTC | 0x20;

      // Wait a full clock cycle to make sure data lines are ready
      // (important for faster clocks)
      // Wait while clock is low
      while (!(PIND & 0x10));

      // Wait for a falling edge of the clock
      while ((PIND & 0x10));

      // Wait while clock is low
      while (!(PIND & 0x10));

      uint8_t inputData = (PIND >> 5) | (PINB << 3);

      // Set VMA OFF
      PORTC = PORTC & 0xDF;

      // Wait for a falling edge of the clock
      // Doesn't seem to help  while((PIND & 0x10));

      // Set R/W to LOW
      PORTD = (PORTD & 0xF7);

      // Clear address lines
      PORTC = (PORTC & 0xE0);

      return inputData;
   }

   void WaitClockCycle(int numCycles = 1) {
      for (int count = 0; count < numCycles; count++) {
         // Wait while clock is low
         while (!(PIND & 0x10));

         // Wait for a falling edge of the clock
         while ((PIND & 0x10));
      }
   }

#elif (RPU_OS_HARDWARE_REV == 3)

   // Rev 3 connections
   // Pin D2 = IRQ
   // Pin D3 = CLOCK
   // Pin D4 = VMA
   // Pin D5 = R/W
   // Pin D6-12 = D0-D6
   // Pin D13 = SWITCH
   // Pin D14 = HALT
   // Pin D15 = D7
   // Pin D16-30 = A0-A14

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV 3 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

   void RPU_DataWrite(int address, uint8_t data) {
      // Set data pins to output
      DDRH = DDRH | 0x78;
      DDRB = DDRB | 0x70;
      DDRJ = DDRJ | 0x01;

      // Set R/W to LOW
      PORTE = (PORTE & 0xF7);

      // Put data on pins
      // Lower Nibble goes on PortH3 through H6
      PORTH = (PORTH & 0x87) | ((data & 0x0F) << 3);
      // Bits 4-6 go on PortB4 through B6
      PORTB = (PORTB & 0x8F) | ((data & 0x70));
      // Bit 7 goes on PortJ0
      PORTJ = (PORTJ & 0xFE) | (data >> 7);

      // Set up address lines
      PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
      PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) | ((address & 0x0010) >> 3) |
              ((address & 0x0020) >> 5); // A2-A5
      PORTA = ((address & 0x3FC0) >> 6); // A6-A13
      PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

      // Wait for a falling edge of the clock
      while ((PINE & 0x20));

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTG = PORTG | 0x20;

      // Wait while clock is low
      while (!(PINE & 0x20));

      // Wait while clock is high
      while ((PINE & 0x20));

      // Wait while clock is low
      while (!(PINE & 0x20));

      // Set VMA OFF
      PORTG = PORTG & 0xFD;

      // Unset address lines
      PORTH = (PORTH & 0xFC);
      PORTD = (PORTD & 0xF0);
      PORTA = 0;
      PORTC = (PORTC & 0x3F);

      // Set R/W back to HIGH
      PORTE = (PORTE | 0x08);

      // Set data pins to input
      DDRH = DDRH & 0x87;
      DDRB = DDRB & 0x8F;
      DDRJ = DDRJ & 0xFE;
   }

   uint8_t RPU_DataRead(int address) {
      // Set data pins to input
      DDRH = DDRH & 0x87;
      DDRB = DDRB & 0x8F;
      DDRJ = DDRJ & 0xFE;

      // Set R/W to HIGH
      DDRE = DDRE | 0x08;
      PORTE = (PORTE | 0x08);

      // Set up address lines
      PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
      PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) | ((address & 0x0010) >> 3) |
              ((address & 0x0020) >> 5); // A2-A5
      PORTA = ((address & 0x3FC0) >> 6); // A6-A13
      PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

      // Set clock low
      PORTE &= ~0x20;

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTG = PORTG | 0x20;

      // Set clock high
      PORTE |= 0x20;

      // Set clock low
      PORTE &= ~0x20;

      // Set clock high
      PORTE |= 0x20;

      uint8_t inputData;
      inputData = (PINH & 0x78) >> 3;
      inputData |= (PINB & 0x70);
      inputData |= PINJ << 7;

      // Set VMA OFF
      PORTG = PORTG & 0xFD;

      // Set R/W to LOW
      PORTE = (PORTE & 0xDF);

      // Unset address lines
      PORTH = (PORTH & 0xFC);
      PORTD = (PORTD & 0xF0);
      PORTA = 0;
      PORTC = (PORTC & 0x3F);

      return inputData;
   }

   void WaitClockCycle(int numCycles = 1) {
      for (int count = 0; count < numCycles; count++) {
         // Wait while clock is low
         while (!(PINE & 0x20));

         // Wait for a falling edge of the clock
         while ((PINE & 0x20));
      }
   }

#elif (RPU_OS_HARDWARE_REV == 4)

   // Rev 3 connections
   // Pin D2 = IRQ
   // Pin D3 = CLOCK
   // Pin D4 = VMA
   // Pin D5 = R/W
   // Pin D6-12 = D0-D6
   // Pin D13 = SWITCH
   // Pin D14 = HALT
   // Pin D15 = D7
   // Pin D16-30 = A0-A14

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV 4 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#define RPU_VMA_PIN 40
#define RPU_RW_PIN 3
#define RPU_PHI2_PIN 39
#define RPU_SWITCH_PIN 38
#define RPU_BUFFER_DISABLE 5
#define RPU_HALT_PIN 41
#define RPU_RESET_PIN 42
#define RPU_DIAGNOSTIC_PIN 44
#define RPU_PINS_OUTPUT true
#define RPU_PINS_INPUT false

   void RPU_SetAddressPinsDirection(bool pinsOutput) {
      for (int count = 0; count < 16; count++) {
         pinMode(A0 + count, pinsOutput ? OUTPUT : INPUT);
      }
   }

   void RPU_SetDataPinsDirection(bool pinsOutput) {
      for (int count = 0; count < 8; count++) {
         pinMode(22 + count, pinsOutput ? OUTPUT : INPUT);
      }
   }

   // REVISION 4 HARDWARE
   void RPU_DataWrite(int address, uint8_t data) {
      // Set data pins to output
      DDRA = 0xFF;

      // Set R/W to LOW
      PORTE = (PORTE & 0xDF);

      // Put data on pins
      PORTA = data;

      // Set up address lines
      PORTF = static_cast<uint8_t>(address & 0x00FF);
      PORTK = static_cast<uint8_t>(address / 256);

      if (UsesM6800Processor) {
         // Wait for a falling edge of the clock
         while ((PING & 0x04));
      } else {
         // Set clock low (PG2) (if 6802/8)
         PORTG &= ~0x04;
      }

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTG = PORTG | 0x02;

      if (UsesM6800Processor) {
         // Wait while clock is low
         while (!(PING & 0x04));

         // Wait while clock is high
         while ((PING & 0x04));

         // Wait while clock is low
         while (!(PING & 0x04));
      } else {
         // Set clock high
         PORTG |= 0x04;

         // Set clock low
         PORTG &= ~0x04;

         // Set clock high
         PORTG |= 0x04;
      }

      // Set VMA OFF
      PORTG = PORTG & 0xFD;

      // Unset address lines
      PORTF = 0x00;
      PORTK = 0x00;

      // Set R/W back to HIGH
      PORTE = (PORTE | 0x20);

      // Set data pins to input
      DDRA = 0x00;
   }

   uint8_t RPU_DataRead(int address) {
      // Set data pins to input
      DDRA = 0x00;

      // Set R/W to HIGH
      DDRE = DDRE | 0x20;
      PORTE = (PORTE | 0x20);

      // Set up address lines
      PORTF = static_cast<uint8_t>(address & 0x00FF);
      PORTK = static_cast<uint8_t>(address / 256);

      if (UsesM6800Processor) {
         // Wait for a falling edge of the clock
         while ((PING & 0x04));
      } else {
         // Set clock low
         PORTG &= ~0x04;
      }

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTG = PORTG | 0x02;

      if (UsesM6800Processor) {
         // Wait a full clock cycle to make sure data lines are ready
         // (important for faster clocks)
         // Wait while clock is low
         while (!(PING & 0x04));

         // Wait for a falling edge of the clock
         while ((PING & 0x04));

         // Wait while clock is low
         while (!(PING & 0x04));
      } else {
         // Set clock high
         PORTG |= 0x04;

         // Set clock low
         PORTG &= ~0x04;

         // Set clock high
         PORTG |= 0x04;
      }

      uint8_t inputData = PINA;

      // Set VMA OFF
      PORTG = PORTG & 0xFD;

      // Set R/W to LOW
      PORTE = (PORTE & 0xDF);

      // Unset address lines
      PORTF = 0x00;
      PORTK = 0x00;

      return inputData;
   }

#elif (RPU_OS_HARDWARE_REV == 100)

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV 100 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#define RPU_VMA_PIN 4
#define RPU_RW_PIN 5
#define RPU_PHI2_PIN 3
#define RPU_SWITCH_PIN 13
#define RPU_BUFFER_DISABLE 2
#define RPU_HALT_PIN 14
#define RPU_RESET_PIN 14
#define RPU_PINS_OUTPUT true
#define RPU_PINS_INPUT false

   void RPU_SetAddressPinsDirection(bool pinsOutput) {
      for (int count = 0; count < 16; count++) {
         pinMode(16 + count, pinsOutput ? OUTPUT : INPUT);
      }
   }

   void RPU_SetDataPinsDirection(bool pinsOutput) {
      for (int count = 0; count < 7; count++) {
         pinMode(6 + count, pinsOutput ? OUTPUT : INPUT);
      }
      pinMode(15, pinsOutput ? OUTPUT : INPUT);
   }

   // REV 100 HARDWARE
   void RPU_DataWrite(int address, uint8_t data) {
      // Set data pins to output
      DDRH = DDRH | 0x78;
      DDRB = DDRB | 0x70;
      DDRJ = DDRJ | 0x01;

      // Set R/W to LOW
      PORTE = (PORTE & 0xF7);

      // Put data on pins
      // Lower Nibble goes on PortH3 through H6
      PORTH = (PORTH & 0x87) | ((data & 0x0F) << 3);
      // Bits 4-6 go on PortB4 through B6
      PORTB = (PORTB & 0x8F) | ((data & 0x70));
      // Bit 7 goes on PortJ0
      PORTJ = (PORTJ & 0xFE) | (data >> 7);

      // Set up address lines
      PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
      PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) | ((address & 0x0010) >> 3) |
              ((address & 0x0020) >> 5); // A2-A5
      PORTA = ((address & 0x3FC0) >> 6); // A6-A13
      PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

      // Set clock low
      PORTE &= ~0x20;

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTG = PORTG | 0x20;

      // Set clock high
      PORTE |= 0x20;

      // Set clock low
      PORTE &= ~0x20;

      // Set clock high
      PORTE |= 0x20;

      // Set VMA OFF
      PORTG = PORTG & 0xFD;

      // Unset address lines
      PORTH = (PORTH & 0xFC);
      PORTD = (PORTD & 0xF0);
      PORTA = 0;
      PORTC = (PORTC & 0x3F);

      // Set R/W back to HIGH
      PORTE = (PORTE | 0x08);

      // Set data pins to input
      DDRH = DDRH & 0x87;
      DDRB = DDRB & 0x8F;
      DDRJ = DDRJ & 0xFE;
   }

   uint8_t RPU_DataRead(int address) {
      // Set data pins to input
      DDRH = DDRH & 0x87;
      DDRB = DDRB & 0x8F;
      DDRJ = DDRJ & 0xFE;

      // Set R/W to HIGH
      DDRE = DDRE | 0x08;
      PORTE = (PORTE | 0x08);

      // Set up address lines
      PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
      PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) | ((address & 0x0010) >> 3) |
              ((address & 0x0020) >> 5); // A2-A5
      PORTA = ((address & 0x3FC0) >> 6); // A6-A13
      PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

      // Set clock low
      PORTE &= ~0x20;

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTG = PORTG | 0x20;

      // Set clock high
      PORTE |= 0x20;

      // Set clock low
      PORTE &= ~0x20;

      // Set clock high
      PORTE |= 0x20;

      uint8_t inputData;
      inputData = (PINH & 0x78) >> 3;
      inputData |= (PINB & 0x70);
      inputData |= PINJ << 7;

      // Set VMA OFF
      PORTG = PORTG & 0xFD;

      // Set R/W to LOW
      PORTE = (PORTE & 0xDF);

      // Unset address lines
      PORTH = (PORTH & 0xFC);
      PORTD = (PORTD & 0xF0);
      PORTA = 0;
      PORTC = (PORTC & 0x3F);

      return inputData;
   }

   void WaitClockCycle(int numCycles = 1) {
      for (int count = 0; count < numCycles; count++) {
         // Wait while clock is low
         while (!(PINE & 0x20));

         // Wait for a falling edge of the clock
         while ((PINE & 0x20));
      }
   }

#elif (RPU_OS_HARDWARE_REV == 101) || (RPU_OS_HARDWARE_REV == 102)

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV >100 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#define RPU_VMA_PIN 40
#define RPU_RW_PIN 3
#define RPU_PHI2_PIN 39
#define RPU_SWITCH_PIN 38
#define RPU_BUFFER_DISABLE 5
#define RPU_HALT_PIN 41
#define RPU_RESET_PIN 42
#define RPU_BA_PIN 43
#define RPU_DIAGNOSTIC_PIN 44
#define RPU_DISABLE_PHI_FROM_MPU 7
#define RPU_DISABLE_PHI_FROM_CPU 6
#define RPU_BOARD_SEL_0 30
#define RPU_BOARD_SEL_1 31
#define RPU_BOARD_SEL_2 32
#define RPU_BOARD_SEL_3 33
#define RPU_PINS_OUTPUT true
#define RPU_PINS_INPUT false

   void RPU_SetAddressPinsDirection(bool pinsOutput) {
      for (int count = 0; count < 16; count++) {
         pinMode(A0 + count, pinsOutput ? OUTPUT : INPUT);
      }
   }

   void RPU_SetDataPinsDirection(bool pinsOutput) {
      for (int count = 0; count < 8; count++) {
         pinMode(22 + count, pinsOutput ? OUTPUT : INPUT);
      }
   }

   // REVISION 101/102 HARDWARE
   void RPU_DataWrite(int address, uint8_t data) {
      // Set data pins to output
      DDRA = 0xFF;

      // Set R/W to LOW
      PORTE = (PORTE & 0xDF);

      // Put data on pins
      PORTA = data;

      // Set up address lines
      PORTF = (uint8_t) (address & 0x00FF);
      PORTK = (uint8_t) (address / 256);

      if (UsesM6800Processor) {
         // Wait for a falling edge of the clock
         while ((PING & 0x04));
      } else {
         // Set clock low (PG2) (if 6802/8)
         PORTG &= ~0x04;
      }

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTG = PORTG | 0x02;

      if (UsesM6800Processor) {
         // Wait while clock is low
         while (!(PING & 0x04));

         // Wait while clock is high
         while ((PING & 0x04));

         // Wait while clock is low
         while (!(PING & 0x04));
      } else {
         // Set clock high
         PORTG |= 0x04;

         // Set clock low
         PORTG &= ~0x04;

         // Set clock high
         PORTG |= 0x04;
      }

      // Set VMA OFF
      PORTG = PORTG & 0xFD;

      // Unset address lines
      PORTF = 0x00;
      PORTK = 0x00;

      // Set R/W back to HIGH
      PORTE = (PORTE | 0x20);

      // Set data pins to input
      DDRA = 0x00;
   }

   uint8_t RPU_DataRead(int address) {
      // Set data pins to input
      DDRA = 0x00;

      // Set R/W to HIGH
      DDRE = DDRE | 0x20;
      PORTE = (PORTE | 0x20);

      // Set up address lines
      PORTF = (uint8_t) (address & 0x00FF);
      PORTK = (uint8_t) (address / 256);

      if (UsesM6800Processor) {
         // Wait for a falling edge of the clock
         while ((PING & 0x04));
      } else {
         // Set clock low
         PORTG &= ~0x04;
      }

      // Pulse VMA over one clock cycle
      // Set VMA ON
      PORTG = PORTG | 0x02;

      if (UsesM6800Processor) {
         // Wait a full clock cycle to make sure data lines are ready
         // (important for faster clocks)
         // Wait while clock is low
         while (!(PING & 0x04));

         // Wait for a falling edge of the clock
         while ((PING & 0x04));

         // Wait while clock is low
         while (!(PING & 0x04));
      } else {
         // Set clock high
         PORTG |= 0x04;

         // Set clock low
         PORTG &= ~0x04;

         // Set clock high
         PORTG |= 0x04;
      }

      uint8_t inputData = PINA;

      // Set VMA OFF
      PORTG = PORTG & 0xFD;

      // Set R/W to LOW
      PORTE = (PORTE & 0xDF);

      // Unset address lines
      PORTF = 0x00;
      PORTK = 0x00;

      return inputData;
   }

   void WaitClockCycle(int numCycles = 1) {
      for (int count = 0; count < numCycles; count++) {
         // Wait while clock is low
         while (!(PINE & 0x20));

         // Wait for a falling edge of the clock
         while ((PINE & 0x20));
      }
   }

#endif

#if (RPU_MPU_ARCHITECTURE < 10)

   void TestLightOn() {
      RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
   }

   void TestLightOff() {
      RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
   }

   void InitializeU10PIA() {
      // CA1 - Self Test Switch
      // CB1 - zero crossing detector
      // CA2 - NOR'd with display latch strobe
      // CB2 - lamp strobe 1
      // PA0-7 - output for switch bank, lamps, and BCD
      // PB0-7 - switch returns

      RPU_DataWrite(ADDRESS_U10_A_CONTROL, 0x38);
      // Set up U10A as output
      RPU_DataWrite(ADDRESS_U10_A, 0xFF);
      // Set bit 3 to write data
      RPU_DataWrite(ADDRESS_U10_A_CONTROL, RPU_DataRead(ADDRESS_U10_A_CONTROL) | 0x04);
      // Store F0 in U10A Output
      RPU_DataWrite(ADDRESS_U10_A, 0xF0);

      RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x33);
      // Set up U10B as input
      RPU_DataWrite(ADDRESS_U10_B, 0x00);
      // Set bit 3 so future reads will read data
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x04);
   }


   void InitializeU11PIA() {
      // CA1 - Display interrupt generator
      // CB1 - test connector pin 32
      // CA2 - lamp strobe 2
      // CB2 - solenoid bank select
      // PA0-7 - display digit enable
      // PB0-7 - solenoid data

      RPU_DataWrite(ADDRESS_U11_A_CONTROL, 0x31);
      // Set up U11A as output
      RPU_DataWrite(ADDRESS_U11_A, 0xFF);
      // Set bit 3 to write data
      RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x04);
      // Store 00 in U11A Output
      RPU_DataWrite(ADDRESS_U11_A, 0x00);

      RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x30);
      // Set up U11B as output
      RPU_DataWrite(ADDRESS_U11_B, 0xFF);
      // Set bit 3 so future reads will read data
      RPU_DataWrite(ADDRESS_U11_B_CONTROL, RPU_DataRead(ADDRESS_U11_B_CONTROL) | 0x04);
      // Store 9F in U11B Output
      RPU_DataWrite(ADDRESS_U11_B, DEFAULT_SOLENOID_STATE);
      SolenoidManager::setCurrentSolenoidByte(DEFAULT_SOLENOID_STATE);
   }

   unsigned long RPU_TestPIAs() {
      unsigned long piaErrors = 0;

      uint8_t piaResult = RPU_DataRead(ADDRESS_U10_A_CONTROL);
      if (piaResult != 0x3C) {
         piaErrors |= RPU_RET_U10_PIA_ERROR;
      }
      piaResult = RPU_DataRead(ADDRESS_U10_B_CONTROL);
      if (piaResult != 0x37) {
         piaErrors |= RPU_RET_U10_PIA_ERROR;
      }

      piaResult = RPU_DataRead(ADDRESS_U11_A_CONTROL);
      if (piaResult != 0x35) {
         piaErrors |= RPU_RET_U11_PIA_ERROR;
      }
      piaResult = RPU_DataRead(ADDRESS_U11_B_CONTROL);
      if (piaResult != 0x34) {
         piaErrors |= RPU_RET_U11_PIA_ERROR;
      }

      return piaErrors;
   }

#else

   void RPU_InitializePIAs() {
      RPU_DataWrite(PIA_DISPLAY_CONTROL_A, 0x31);
      RPU_DataWrite(PIA_DISPLAY_PORT_A, 0xFF);
      RPU_DataWrite(PIA_DISPLAY_CONTROL_A, 0x3D);
      RPU_DataWrite(PIA_DISPLAY_PORT_A, 0xC0);

      RPU_DataWrite(PIA_DISPLAY_CONTROL_B, 0x31);
      RPU_DataWrite(PIA_DISPLAY_PORT_B, 0xFF);
      RPU_DataWrite(PIA_DISPLAY_CONTROL_B, 0x3D);
      RPU_DataWrite(PIA_DISPLAY_PORT_B, 0x00);

      RPU_DataWrite(PIA_SWITCH_CONTROL_A, 0x38);
      RPU_DataWrite(PIA_SWITCH_PORT_A, 0x00);
      RPU_DataWrite(PIA_SWITCH_CONTROL_A, 0x3C);

      RPU_DataWrite(PIA_SWITCH_CONTROL_B, 0x38);
      RPU_DataWrite(PIA_SWITCH_PORT_B, 0xFF);
      RPU_DataWrite(PIA_SWITCH_CONTROL_B, 0x3C);
      RPU_DataWrite(PIA_SWITCH_PORT_B, 0x00);

      RPU_DataWrite(PIA_LAMPS_CONTROL_A, 0x38);
      RPU_DataWrite(PIA_LAMPS_PORT_A, 0xFF);
      RPU_DataWrite(PIA_LAMPS_CONTROL_A, 0x3C);
      RPU_DataWrite(PIA_LAMPS_PORT_A, 0xFF);

      RPU_DataWrite(PIA_LAMPS_CONTROL_B, 0x38);
      RPU_DataWrite(PIA_LAMPS_PORT_B, 0xFF);
      RPU_DataWrite(PIA_LAMPS_CONTROL_B, 0x3C);
      RPU_DataWrite(PIA_LAMPS_PORT_B, 0x00);

#if (RPU_MPU_ARCHITECTURE < 15)
   RPU_DataWrite(PIA_SOLENOID_CONTROL_A, 0x38);
   RPU_DataWrite(PIA_SOLENOID_PORT_A, 0xFF);
   RPU_DataWrite(PIA_SOLENOID_CONTROL_A, 0x3C);
#endif
   RPU_DataWrite(PIA_SOLENOID_PORT_A, 0x00);

#if (RPU_MPU_ARCHITECTURE < 15)
   RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x30);
   RPU_DataWrite(PIA_SOLENOID_PORT_B, 0xFF);
   RPU_DataWrite(PIA_SOLENOID_CONTROL_B, 0x34);
   RPU_DataWrite(PIA_SOLENOID_PORT_B, 0x00);
#endif

#if (RPU_MPU_ARCHITECTURE == 15)
   RPU_DataWrite(PIA_SOLENOID_11_CONTROL_B, 0x38);
   RPU_DataWrite(PIA_SOLENOID_11_PORT_B, 0xFF);
   RPU_DataWrite(PIA_SOLENOID_11_CONTROL_B, 0x3C);
   RPU_DataWrite(PIA_SOLENOID_11_PORT_B, 0x00);

   RPU_DataWrite(PIA_ALPHA_DISPLAY_CONTROL_A, 0x38);
   RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_A, 0xFF);
   RPU_DataWrite(PIA_ALPHA_DISPLAY_CONTROL_A, 0x3C);
   RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_A, 0x00);

   RPU_DataWrite(PIA_ALPHA_DISPLAY_CONTROL_B, 0x38);
   RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_B, 0xFF);
   RPU_DataWrite(PIA_ALPHA_DISPLAY_CONTROL_B, 0x3C);
   RPU_DataWrite(PIA_ALPHA_DISPLAY_PORT_B, 0x00);

   RPU_DataWrite(PIA_NUM_DISPLAY_CONTROL_A, 0x38);
   RPU_DataWrite(PIA_NUM_DISPLAY_PORT_A, 0xFF);
   RPU_DataWrite(PIA_NUM_DISPLAY_CONTROL_A, 0x3C);
   RPU_DataWrite(PIA_NUM_DISPLAY_PORT_A, 0x00);

   RPU_DataWrite(PIA_SOUND_11_CONTROL_A, 0x38);
   RPU_DataWrite(PIA_SOUND_11_PORT_A, 0xFF);
   RPU_DataWrite(PIA_SOUND_11_CONTROL_A, 0x3C);
   RPU_DataWrite(PIA_SOUND_11_PORT_A, 0x00);

   RPU_DataWrite(PIA_WIDGET_CONTROL_B, 0x38);
   RPU_DataWrite(PIA_WIDGET_PORT_B, 0xFF);
   RPU_DataWrite(PIA_WIDGET_CONTROL_B, 0x3C);
   RPU_DataWrite(PIA_WIDGET_PORT_B, 0x00);
#endif

#if (RPU_MPU_ARCHITECTURE == 13)
   RPU_DataWrite(PIA_SOUND_COMMA_CONTROL_A, 0x38);
   RPU_DataWrite(PIA_SOUND_COMMA_PORT_A, 0xFF);
   RPU_DataWrite(PIA_SOUND_COMMA_CONTROL_A, 0x3C);
   RPU_DataWrite(PIA_SOUND_COMMA_PORT_A, 0x00);

   RPU_DataWrite(PIA_SOUND_COMMA_CONTROL_B, 0x38);
   RPU_DataWrite(PIA_SOUND_COMMA_PORT_B, 0xFF);
   RPU_DataWrite(PIA_SOUND_COMMA_CONTROL_B, 0x3C);
   RPU_DataWrite(PIA_SOUND_COMMA_PORT_B, 0x00);
#endif
   }

   unsigned long RPU_TestPIAs() {
      unsigned long piaErrors = 0;

      uint8_t piaResult = RPU_DataRead(PIA_DISPLAY_CONTROL_A);
      if (piaResult != 0x3D) {
         piaErrors |= RPU_RET_PIA_1_ERROR;
         RPU_DEBUG_MESSAGE("* Error with Display PIA\n");
      } else {
         RPU_DEBUG_MESSAGE("* No error with Display PIA\n");
      }
      piaResult = RPU_DataRead(PIA_DISPLAY_CONTROL_B);
      if (piaResult != 0x3D) {
         piaErrors |= RPU_RET_PIA_1_ERROR;
      }

      piaResult = RPU_DataRead(PIA_SWITCH_CONTROL_A);
      if (piaResult != 0x3C) {
         piaErrors |= RPU_RET_PIA_2_ERROR;
      }
      piaResult = RPU_DataRead(PIA_SWITCH_CONTROL_B);
      if (piaResult != 0x3C) {
         piaErrors |= RPU_RET_PIA_2_ERROR;
      }

      piaResult = RPU_DataRead(PIA_LAMPS_CONTROL_A);
      if (piaResult != 0x3C) {
         piaErrors |= RPU_RET_PIA_3_ERROR;
      }
      piaResult = RPU_DataRead(PIA_LAMPS_CONTROL_B);
      if (piaResult != 0x3C) {
         piaErrors |= RPU_RET_PIA_3_ERROR;
      }

      piaResult = RPU_DataRead(PIA_SOLENOID_CONTROL_A);
      if (piaResult != 0x3C) {
         piaErrors |= RPU_RET_PIA_4_ERROR;
      }
      piaResult = RPU_DataRead(PIA_SOLENOID_CONTROL_B);
      if (piaResult != 0x3C) {
         piaErrors |= RPU_RET_PIA_4_ERROR;
      }

#if (RPU_MPU_ARCHITECTURE == 13)
   piaResult= RPU_DataRead(PIA_SOUND_COMMA_CONTROL_A);
   if (piaResult!= 0x3C) {
      piaErrors |= RPU_RET_PIA_5_ERROR;
   }
   piaResult= RPU_DataRead(PIA_SOUND_COMMA_CONTROL_B);
   if (piaResult!= 0x3C) {
      piaErrors |= RPU_RET_PIA_5_ERROR;
   }
#endif

   return piaErrors;
}

   void RPU_SetBoardLEDs(bool LED1, bool LED2, uint8_t BCDValue) {
      uint8_t leds = 0;
      if (BCDValue == 0xFF) {
         if (LED1) leds |= 0x20;
         if (LED2) leds |= 0x10;
      } else {
         leds = BCDValue * 16;
      }
      SolenoidManager::setBoardLEDs(leds);
   }

#endif

   /******************************************************
    *   Switch Handling Functions
    */

   void RPU_PushToSwitchStack(uint8_t switchNumber) {
      SwitchManager::getInstance().push(switchNumber);
   }

   uint8_t RPU_PullFirstFromSwitchStack() {
      return SwitchManager::getInstance().pull();
   }

   bool RPU_ReadSingleSwitchState(uint8_t switchNum) {
      return SwitchManager::getInstance().readState(switchNum);
   }

   void RPU_SetupGameSwitches(int s_numSwitches, int s_numPrioritySwitches,
                              const PlayfieldAndCabinetSwitch *s_gameSwitchArray) {
      SwitchManager::getInstance().setupGameSwitches(s_numSwitches, s_numPrioritySwitches, s_gameSwitchArray);
   }

   void RPU_ClearUpDownSwitchState() {
      SwitchManager::getInstance().clearUpDownSwitchState();
   }

   bool RPU_GetUpDownSwitchState() {
      return SwitchManager::getInstance().getUpDownSwitchState();
   }

   /******************************************************
    *   Solenoid Handling Functions
    */

   void RPU_PushToSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes, bool disableOverride) {
      if (solenoidNumber >= RPU_NUM_SOLENOIDS) return;
      SolenoidManager::getInstance().push(solenoidNumber, numPushes, disableOverride);
   }

   bool RPU_PushToTimedSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes,
                                     unsigned long whenToFire, bool disableOverride) {
      return SolenoidManager::getInstance().pushTimed(solenoidNumber, numPushes, whenToFire, disableOverride);
   }

   void RPU_UpdateTimedSolenoidStack(unsigned long curTime) {
      SolenoidManager::getInstance().updateTimed(curTime);
   }

   void RPU_SetCoinLockout(bool lockoutOff, uint8_t solbit) {
      SolenoidManager::getInstance().setCoinLockout(lockoutOff, solbit);
   }

   void RPU_SetDisableFlippers(bool disableFlippers, uint8_t solbit) {
      SolenoidManager::getInstance().setDisableFlippers(disableFlippers, solbit);
   }

#if (RPU_MPU_ARCHITECTURE < 10)
   void RPU_SetContinuousSolenoidBit(bool bitOn, uint8_t solbit) {
      SolenoidManager::getInstance().setContinuousSolenoidBit(bitOn, solbit);
   }

   bool RPU_FireContinuousSolenoid(uint8_t solBit, uint8_t numCyclesToFire) {
      return SolenoidManager::getInstance().fireContinuousSolenoid(solBit, numCyclesToFire);
   }
#else
   void RPU_SetContinuousSolenoid(bool solOn, uint8_t solNum) {
      SolenoidManager::getInstance().setContinuousSolenoid(solOn, solNum);
   }
#endif

   uint8_t RPU_ReadContinuousSolenoids() {
      return SolenoidManager::getInstance().readContinuousSolenoids();
   }

   void RPU_DisableSolenoidStack() {
      SolenoidManager::getInstance().disableStack();
   }

   void RPU_EnableSolenoidStack() {
      SolenoidManager::getInstance().enableStack();
   }

   /******************************************************
    *   Lamp Handling Functions
    */

   void RPU_SetDimDivisor(uint8_t level, uint8_t divisor) {
      LampManager::getInstance().setDimDivisor(level, divisor);
   }

   // left shift is iterative on Arduinos, so a bit array is suprisingly faster
   const uint8_t BitShiftValues[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

   void RPU_SetLampState(int lampNum, uint8_t s_lampState, uint8_t s_lampDim, int s_lampFlashPeriod) {
      LampManager::getInstance().setLampState(lampNum, s_lampState, s_lampDim, s_lampFlashPeriod);
   }

   void RPU_SetLampAnimation(const uint8_t *lamps, unsigned count) {
      LampManager::getInstance().setLampAnimation(lamps, count);
   }

   uint8_t RPU_ReadLampState(int lampNum) {
      return LampManager::getInstance().readLampState(lampNum);
   }

   uint8_t RPU_ReadLampDim(int lampNum) {
      return LampManager::getInstance().readLampDim(lampNum);
   }

   int RPU_ReadLampFlash(int lampNum) {
      return LampManager::getInstance().readLampFlash(lampNum);
   }

   void RPU_ApplyFlashToLamps(unsigned long curTime) {
      LampManager::getInstance().applyFlashToLamps(curTime);
   }

   void RPU_FlashAllLamps(unsigned long curTime) {
      LampManager::getInstance().flashAllLamps(curTime);
   }

   void RPU_TurnOffAllLamps() {
      LampManager::getInstance().turnOffAllLamps();
   }

   void RPU_ClearVariables() {
      // Turn off all lamp states
      LampManager::getInstance().clearLampStates();
   }

#if (RPU_MPU_ARCHITECTURE < 10)

   // ISR: fires at RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL rate (~320 Hz).
   // Each call advances one display × digit position and drives lamps.
   //
   // Display mux format for Stern arch-1 (needs verification against Trident
   // schematics before first hardware test):
   //   U10A write: upper nibble = display index, lower nibble = BCD digit value
   //   U11A write: bit mask with 1 bit set for the active digit column (0-5)
   //   U10A CA2 strobe latches the display data into the board's shift register.
   ISR(TIMER1_COMPA_vect) {
      static uint8_t displayState = 0; // 0 .. (5 * RPU_OS_NUM_DIGITS - 1)
      static int     interruptCount = 0;
      interruptCount++;

      uint8_t disp = displayState / RPU_OS_NUM_DIGITS;
      uint8_t dig  = displayState % RPU_OS_NUM_DIGITS;

      // Blank digit output before switching to avoid ghosting
      RPU_DataWrite(ADDRESS_U11_A, 0x00);

      volatile uint8_t* digits = DisplayManager::getDisplayDigits(disp);
      uint8_t bcdVal = digits[dig] & 0x0F;

      // Write display select + BCD to shared data bus
      RPU_DataWrite(ADDRESS_U10_A, (uint8_t)((disp << 4) | bcdVal));

      // Strobe display latch via CA2 (set then clear bit 3 of control register)
      RPU_DataWrite(ADDRESS_U10_A_CONTROL,
                    RPU_DataRead(ADDRESS_U10_A_CONTROL) | 0x08);
      RPU_DataWrite(ADDRESS_U10_A_CONTROL,
                    RPU_DataRead(ADDRESS_U10_A_CONTROL) & 0xF7);

      // Enable digit column if the digit-enable mask includes this position
      uint8_t enableMask = DisplayManager::getDisplayDigitEnable(disp);
      if (enableMask & (1 << dig)) {
         RPU_DataWrite(ADDRESS_U11_A, (uint8_t)(1 << dig));
      }

      displayState++;
      if (displayState >= 5 * RPU_OS_NUM_DIGITS) displayState = 0;

      // Lamp multiplexing runs on every interrupt
      LampManager::getInstance().multiplexLamps(interruptCount);
   }

   unsigned long RPU_InitializeMPU(unsigned long initOptions,
                                   uint8_t       creditResetSwitch) {
      (void)initOptions;
      (void)creditResetSwitch;

      InitializeU10PIA();
      InitializeU11PIA();

      // Timer1: CTC mode, prescaler 1024
      // Fires at 16 MHz / 1024 / (RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL + 1) ≈ 319 Hz
      cli();
      TCCR1A = 0;
      TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
      OCR1A  = RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL;
      TIMSK1 |= (1 << OCIE1A);
      sei();

      return RPU_RET_NO_ERRORS;
   }

#endif // RPU_MPU_ARCHITECTURE < 10

   // Stub for architectures >= 10 (not needed for Milestone 1)
#if (RPU_MPU_ARCHITECTURE >= 10)
   unsigned long RPU_InitializeMPU(unsigned long initOptions,
                                   uint8_t       creditResetSwitch) {
      (void)initOptions;
      (void)creditResetSwitch;
      RPU_InitializePIAs();
      return RPU_RET_NO_ERRORS;
   }
#endif

   void RPU_Update(unsigned long currentTime) {
      (void)currentTime;
   }

