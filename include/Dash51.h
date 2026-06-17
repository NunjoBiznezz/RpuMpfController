#ifndef DASH51_H
#define DASH51_H

#include <stdint.h>

namespace RPU {

#ifdef RPU_OS_USE_DASH51
   void RPU_PlaySoundDash51(uint8_t soundByte);
#endif

} // namespace RPU

#endif  // DASH51_H
