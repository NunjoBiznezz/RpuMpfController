#pragma once

// LISY serial protocol command bytes.
// MPF sends one of these as the first byte of every message; the RPU responds.
// Source of truth: mpf/platforms/lisy/defines.py in the MPF source.
//
// All communication is MPF-initiated. The RPU never sends unsolicited data.
// Baud rate: 115200.

// --- Info / startup (MPF queries these in order on connect) ---
#define LISY_INFO_GET_HARDWARE          0   // RPU responds: null-terminated string e.g. "LISY80\0"
#define LISY_INFO_LISY_VERSION          1   // RPU responds: null-terminated version string e.g. "4.02\0"
#define LISY_INFO_GET_API_VERSION       2   // RPU responds: null-terminated version string e.g. "0.9\0"
#define LISY_INFO_GET_NUM_LAMPS         3   // RPU responds: 1 byte count
#define LISY_INFO_GET_NUM_SOLENOIDS     4   // RPU responds: 1 byte count
#define LISY_INFO_GET_NUM_SOUNDS        5   // RPU responds: 1 byte count (return 0)
#define LISY_INFO_GET_NUM_DISPLAYS      6   // RPU responds: 1 byte count
#define LISY_INFO_GET_DISPLAY_DETAILS   7   // RPU responds: 2 bytes [type, digit_count]
                                            //   type 3 = 7-segment (used by Trident)
#define LISY_INFO_GET_GAME_INFO         8   // not used
#define LISY_INFO_GET_SWITCH_COUNT      9   // RPU responds: 1 byte count

// --- Lamps (no response) ---
#define LISY_LAMPS_GET_STATUS           10  // not used
#define LISY_LAMP_ON                    11  // payload: [lamp_num]
#define LISY_LAMP_OFF                   12  // payload: [lamp_num]
#define LISY_FADE_MODERN_LIGHTS         13  // modern RGB LEDs only, not needed for Trident

#define LISY_GET_MODERN_LIGHTS_COUNT    19  // RPU responds: 1 byte count (return 0 for Trident)

// --- Solenoids (no response) ---
#define LISY_SOL_GET_STATUS             20  // not used
#define LISY_SOL_ON                     21  // payload: [solenoid_num]
#define LISY_SOL_OFF                    22  // payload: [solenoid_num]
#define LISY_SOL_PULSE                  23  // payload: [solenoid_num]  — fires for configured pulse_ms
#define LISY_SOL_SET_PULSE_TIME         24  // payload: [solenoid_num, pulse_ms]
#define LISY_SOL_SET_RECYCLE_TIME       25  // payload: [solenoid_num, recycle_ms]
#define LISY_SOL_PULSE_WITH_PWM         26  // not used

// --- Displays (no response, API >= 0.9) ---
// Command byte = 30 + display_index (displays 0..6)
// Payload: [byte_count, ...segment_bytes...]
// For 7-segment type 3: each byte encodes one digit in dp-g-f-e-d-a-b-c bit order
#define LISY_DISPLAY_0                  30
#define LISY_DISPLAY_1                  31
#define LISY_DISPLAY_2                  32
#define LISY_DISPLAY_3                  33
#define LISY_DISPLAY_4                  34
#define LISY_DISPLAY_5                  35
#define LISY_DISPLAY_6                  36

// --- Switches ---
#define LISY_SWITCH_GET_STATUS          40  // payload: [switch_num]
                                            // RPU responds: 0=inactive, 1=active, 2=doesn't exist
#define LISY_SWITCH_GET_CHANGED         41  // no payload
                                            // RPU responds: 0x7F if no changes
                                            //   otherwise 1 byte: bit7=state, bits0-6=switch_num

// --- Sound (not needed for original Trident chime board) ---
#define LISY_SOUND_PLAY                 50
#define LISY_SOUND_STOP_ALL             51
#define LISY_SOUND_PLAY_FILE            52
#define LISY_SOUND_TTS                  53
#define LISY_SOUND_SET_VOLUME           54

// --- Hardware rules (flipper logic offloaded to RPU) ---
#define LISY_HW_RULE_SOLENOID           60  // 10-byte payload

// --- General ---
#define LISY_RESET                      100 // RPU responds: 0x00
#define LISY_WATCHDOG                   101 // RPU responds: 0x00  (called every 500ms by MPF)

// Display type codes (returned in response to LISY_INFO_GET_DISPLAY_DETAILS)
#define LISY_DISPLAY_TYPE_BCD           1
#define LISY_DISPLAY_TYPE_BCD_DP        2
#define LISY_DISPLAY_TYPE_7SEG          3   // Stern Trident uses this
#define LISY_DISPLAY_TYPE_14SEG         4
#define LISY_DISPLAY_TYPE_ASCII         5
#define LISY_DISPLAY_TYPE_ASCII_DP      6
