/**************************************************************************
 *     This file is part of the RPU OS for Arduino Project.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 6/1/2020

    RPU OS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPU OS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#pragma once

/***
  Use this file to set game_-specific and hardware-specific parameters
***/

// Board dispatch — exactly one BOARD_* token must be defined per environment
#if defined(BOARD_RPU_V3)
#  include "boards/rpu_v3.h"
#elif defined(BOARD_RPU_V4)
#  include "boards/rpu_v4.h"
#elif defined(BOARD_RPU_V102)
#  include "boards/rpu_v102.h"
#else
#  error "No board defined. Add -DBOARD_RPU_V3, -DBOARD_RPU_V4, or -DBOARD_RPU_V102 to your environment."
#endif

// Machine dispatch — exactly one MACHINE_* token must be defined per environment
#if defined(MACHINE_STERN_TRIDENT)
#  include "machines/stern_trident.h"
#else
#  error "No machine defined. Add -DMACHINE_STERN_TRIDENT (or another machine token) to your environment."
#endif

#include <stdint.h>

namespace RPU {
   /*
    *   Plug-in board architectures (0-99 is for J5, 100-199 is for CPU socket)
    *
    *   RPU_OS_HARDWARE_REV 1 - Nano board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
    *                            generally uses an Arduino Nano & (optional) 74125
    *
    *   RPU_OS_HARDWARE_REV 2 - Nano board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
    *                           adds support for SB300 sound cards
    *                           uses an Arduino Nano, a 74155, and a 74240
    *
    *   RPU_OS_HARDWARE_REV 3 - MEGA2560 PRO board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
    *                           adds support for full address space
    *                           uses a MEGA 2560 Pro, and nothing else
    *
    *   RPU_OS_HARDWARE_REV 4 - MEGA2560 PRO board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
    *                           adds support for OLED display, WIFI, and multiple serial ports
    *                           uses a MEGA 2560 Pro (all the pins) on a larger board (display & WIFI)
    *
    *   RPU_OS_HARDWARE_REV 100 - MEGA2560 PRO board that plugs into processor socket (prototype)
    *                            (different order of magnitude because it's a different approach) plugs into
    *                            the CPU socket
    *
    *   RPU_OS_HARDWARE_REV 101 - MEGA2560 PRO board that plugs into processor socket
    *                             adds support for multiple serial ports (limited release)
    *                             first RPU CPU interposer release board
    *
    *   RPU_OS_HARDWARE_REV 102 - MEGA2560 PRO board that plugs into processor socket (prototype)
    *                             adds support for OLED display, WIFI, autodetection of processor type
    *                             second RPU (with display and WIFI socket)
    */

#if !defined(RPU_OS_HARDWARE_REV)
#error "RPU_OS_HARDWARE_REV not defined. Set a BOARD_* token (e.g. -DBOARD_RPU_V102) in your platformio.ini environment."
#endif

// Available Architectures (0-9 is for B/S Boards, 10-19 is for W)
//  RPU_MPU_ARCHITECTURE 1 = -17, -35, 100, 200, or compatible
//  RPU_MPU_ARCHITECTURE 11 = Sys 4, 6
//  RPU_MPU_ARCHITECTURE 13 = Sys 7
//  RPU_MPU_ARCHITECTURE 15 = Sys 11
#if !defined(RPU_MPU_ARCHITECTURE)
#error "RPU_MPU_ARCHITECTURE not defined. Set a BOARD_* token (e.g. -DBOARD_RPU_V102) in your platformio.ini environment."
#endif

// Some boards will assume a 6800 is the processor (RPU_OS_HARDWARE_REV 1 through 4)
// and some boards will try to detect the processor (RPU_OS_HARDWARE_REV 102)
// but in other cases we can specify if we're building for a 6800.
// Define RPU_MPU_BUILD_FOR_6800 with a 0 for 6802 or 6808, and with
// a 1 for 6800
#if !defined(RPU_MPU_BUILD_FOR_6800)
#error "RPU_MPU_BUILD_FOR_6800 not defined. Set a BOARD_* token (e.g. -DBOARD_RPU_V102) in your platformio.ini environment."
#endif

// These defines allow this configuration to eliminate some functions
// to reduce program size
// Note: These are now controlled by CMake build options. Do not define them here.
// They will be passed as compile definitions by the build system.
// Available options:
//   RPU_OS_USE_DIP_SWITCHES - Enable DIP switch reading
//   RPU_OS_USE_SB100 - Enable SB-100 sound card support
//   RPU_OS_USE_SB300 - Enable SB-300 sound card support (requires hardware rev >= 2)
//   RPU_OS_USE_WAV_TRIGGER_1p3 - Enable WavTrigger 1.3 support
//   RPU_OS_USE_7_DIGIT_DISPLAYS - Enable 7-digit displays
//   RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS - Use 6-digit credit with 7-digit displays
//   RPU_OS_USE_AUX_LAMPS - Enable auxiliary lamps (88 lamps instead of 60)
//   RPU_OS_USE_S_AND_T - Enable S&T sound card support
//   RPU_OS_USE_DASH51 - Enable Dash-51 sound card support
//   RPU_OS_DISABLE_CPC_FOR_SPACE - Disable CPC code to save space
// #define RPU_USE_EXTENDED_SWITCHES_ON_PB4
// #define RPU_USE_EXTENDED_SWITCHES_ON_PB7
// #define RPU_OS_USE_WTYPE_1_SOUND
// #define RPU_OS_USE_WTYPE_2_SOUND
// #define RPU_OS_USE_W11_SOUND

#if (RPU_MPU_ARCHITECTURE == 1)
/*******************************************************
 * This section is only for games that use the
 * -17, -35, 100, and 200 MPU boards
 */
 // Depending on the number of digits, the RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL
 // can be adjusted in order to change the refresh rate of the displays.
 // The original -17 / MPU-100 boards ran at 320 Hz
 // The Alltek runs the displays at 440 Hz (probably so 7-digit displays won't flicker)
 // The value below is calculated with this formula:
 //       Value = (interval in ms) * (16*10^6) / (1*1024) - 1
 //          (must be <65536)
 // Choose one of these values (or do whatever)
 //  Value         Frequency
 //  48            318.8 Hz
 //  47            325.5 Hz
 //  46            332.4 Hz increments   (I use this for 6-digits displays)
 //  45            339.6 Hz
 //  40            381 Hz
 //  35            434 Hz     (This would probably be good for 7-digit displays)
 //  34            446.4 Hz
 #define RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL 48
 // #define RPU_OS_ADJUSTABLE_DISPLAY_INTERRUPT

 #ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
 constexpr uint8_t RPU_OS_MASK_SHIFT_1 = 0x60;
 constexpr uint8_t RPU_OS_MASK_SHIFT_2 = 0x0C;
 #else
 constexpr uint8_t RPU_OS_MASK_SHIFT_1 = 0x30;
 constexpr uint8_t RPU_OS_MASK_SHIFT_2 = 0x06;
 #endif

 #ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
 constexpr long RPU_OS_MAX_DISPLAY_SCORE = 9999999;
 constexpr int RPU_OS_NUM_DIGITS = 7;
 constexpr uint8_t RPU_OS_ALL_DIGITS_MASK = 0x7F;
 #else
 constexpr long RPU_OS_MAX_DISPLAY_SCORE = 999999;
 constexpr int RPU_OS_NUM_DIGITS = 6;
 constexpr uint8_t RPU_OS_ALL_DIGITS_MASK = 0x3F;
 #endif

 constexpr uint8_t CONTSOL_DISABLE_FLIPPERS = 0x40;
 constexpr uint8_t CONTSOL_DISABLE_COIN_LOCKOUT = 0x20;

 #define RPU_OS_SWITCH_DELAY_IN_MICROSECONDS 200
 #define RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS 70

 // Fast boards might need a slower lamp strobe
 // #define RPU_OS_SLOW_DOWN_LAMP_STROBE  0

 #ifdef RPU_OS_USE_AUX_LAMPS
 constexpr int RPU_NUM_LAMP_BANKS = 11;
 constexpr int RPU_MAX_LAMPS = 88;
 #else
 constexpr int RPU_NUM_LAMP_BANKS = 8;
 constexpr int RPU_MAX_LAMPS = 60;
 #endif

 constexpr int NUM_SWITCH_BYTES = 8;
 constexpr int MAX_NUM_SWITCHES = 64;

#elif (RPU_MPU_ARCHITECTURE > 9)
/*******************************************************
 * This section is only for games that use the
 * System 4, 6, 7, 9 or 11 / other MPU boards
 */

constexpr int RPU_NUM_LAMP_BANKS = 8;
constexpr int RPU_MAX_LAMPS = 64;

constexpr int NUM_SWITCH_BYTES = 8;
constexpr int MAX_NUM_SWITCHES = 64;

#if (RPU_MPU_ARCHITECTURE < 13)
constexpr uint16_t RPU_OS_MAX_DISPLAY_SCORE = 999999;
constexpr uint16_t RPU_OS_NUM_DIGITS = 6;
constexpr uint16_t RPU_OS_ALL_DIGITS_MASK = 0x3F;
#else
constexpr uint16_t RPU_OS_MAX_DISPLAY_SCORE = 9999999;
constexpr uint16_t RPU_OS_NUM_DIGITS = 7;
constexpr uint16_t RPU_OS_ALL_DIGITS_MASK = 0x7F;
#endif

#endif

}
