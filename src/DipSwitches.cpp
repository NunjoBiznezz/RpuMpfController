//
// Created by Douglas Bercot on 5/10/26.
//
#include "RPU_config.h"
#ifdef RPU_OS_USE_DIP_SWITCHES

#include "RPU.h"
#include "RPU_Internals.h"
#include <Arduino.h>


   static uint8_t DipSwitches[4];

   void ReadDipSwitches() {
      uint8_t backupU10A = RPU_DataRead(ADDRESS_U10_A);
      uint8_t backupU10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);

      // Turn on Switch strobe 5 & Read Switches
      RPU_DataWrite(ADDRESS_U10_A, 0x20);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
      // Wait for switch capacitors to charge
      delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
      DipSwitches[0] = RPU_DataRead(ADDRESS_U10_B);

      // Turn on Switch strobe 6 & Read Switches
      RPU_DataWrite(ADDRESS_U10_A, 0x40);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
      // Wait for switch capacitors to charge
      delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
      DipSwitches[1] = RPU_DataRead(ADDRESS_U10_B);

      // Turn on Switch strobe 7 & Read Switches
      RPU_DataWrite(ADDRESS_U10_A, 0x80);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
      // Wait for switch capacitors to charge
      delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
      DipSwitches[2] = RPU_DataRead(ADDRESS_U10_B);

      // Turn on U10 CB2 (strobe 8) and read switches
      RPU_DataWrite(ADDRESS_U10_A, 0x00);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl | 0x08);
      // Wait for switch capacitors to charge
      delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
      DipSwitches[3] = RPU_DataRead(ADDRESS_U10_B);

      RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl);
      RPU_DataWrite(ADDRESS_U10_A, backupU10A);
   }


   uint8_t RPU_GetDipSwitches(uint8_t index) {
      if (index > 3) {
         return 0x00;
      }
      return DipSwitches[index];
   }


#endif
