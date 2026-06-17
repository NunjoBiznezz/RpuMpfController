#include "RPU_config.h"
#if (RPU_OS_HARDWARE_REV >= 2 && defined(RPU_OS_USE_SB300))

#include "RPU.h"
#include "RPU_Internals.h"

//
// Created by Douglas Bercot on 3/25/26.
//


namespace RPU {
   void RPU_PlaySB300SquareWave(uint8_t soundRegister, uint8_t soundByte) {
      RPU_DataWrite(ADDRESS_SB300_SQUARE_WAVES + soundRegister, soundByte);
   }

   void RPU_PlaySB300Analog(uint8_t soundRegister, uint8_t soundByte) {
      RPU_DataWrite(ADDRESS_SB300_ANALOG + soundRegister, soundByte);
   }
}

#endif
