#pragma once

#include <stdint.h>

// Machine peripheral flags
#define RPU_OS_USE_DIP_SWITCHES
#define RPU_OS_USE_SB100
// Trident uses standard 6-digit displays (no RPU_OS_USE_7_DIGIT_DISPLAYS)
// Trident uses standard 60-lamp board (no RPU_OS_USE_AUX_LAMPS)

// Stern Trident (1979) — machine resource counts for the LISY protocol handshake.
// Source: MissionPinball/machine_files/stern_trident.yaml

constexpr uint8_t MACHINE_NUM_SWITCHES   = 34;
constexpr uint8_t MACHINE_NUM_SOLENOIDS  = 15;
constexpr uint8_t MACHINE_NUM_LAMPS      = 60;
constexpr uint8_t MACHINE_NUM_DISPLAYS   = 6;
constexpr uint8_t MACHINE_DISPLAY_TYPE   = 3;  // LISY type 3 = 7-segment
constexpr uint8_t MACHINE_DISPLAY_DIGITS = 6;
