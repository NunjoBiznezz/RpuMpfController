#pragma once

// LISY serial protocol command bytes.
// MPF sends one of these as the first byte of every message; the RPU responds.
// Source of truth: mpf/platforms/lisy/defines.py in the MPF source.
//
// All communication is MPF-initiated. The RPU never sends unsolicited data.
// Baud rate: 115200.

// --- Info / startup (MPF queries these in order on connect) ---
#define LISY_INFO_GET_HARDWARE          0x00  // RPU responds: null-terminated string e.g. "LISY80\0"
#define LISY_INFO_LISY_VERSION          0x01  // RPU responds: null-terminated version string e.g. "4.02\0"
#define LISY_INFO_GET_API_VERSION       0x02  // RPU responds: null-terminated version string e.g. "0.9\0"
#define LISY_INFO_GET_NUM_LAMPS         0x03  // RPU responds: 1 byte count
#define LISY_INFO_GET_NUM_SOLENOIDS     0x04  // RPU responds: 1 byte count
#define LISY_INFO_GET_NUM_SOUNDS        0x05  // RPU responds: 1 byte count (return 0)
#define LISY_INFO_GET_NUM_DISPLAYS      0x06  // RPU responds: 1 byte count
#define LISY_INFO_GET_DISPLAY_DETAILS   0x07  // RPU responds: 2 bytes [type, digit_count]
                                              //   type 3 = 7-segment (used by Trident)
#define LISY_INFO_GET_GAME_INFO         0x08  // not used
#define LISY_INFO_GET_SWITCH_COUNT      0x09  // RPU responds: 1 byte count

// --- Lamps (no response) ---
#define LISY_LAMPS_GET_STATUS           0x0A  // not used
#define LISY_LAMP_ON                    0x0B  // payload: [lamp_num]
#define LISY_LAMP_OFF                   0x0C  // payload: [lamp_num]
#define LISY_FADE_MODERN_LIGHTS         0x0D  // modern RGB LEDs only, not needed for Trident

#define LISY_GET_MODERN_LIGHTS_COUNT    0x13  // RPU responds: 1 byte count (return 0 for Trident)

// --- Solenoids (no response) ---
#define LISY_SOL_GET_STATUS             0x14  // not used
#define LISY_SOL_ON                     0x15  // payload: [solenoid_num]
#define LISY_SOL_OFF                    0x16  // payload: [solenoid_num]
#define LISY_SOL_PULSE                  0x17  // payload: [solenoid_num]  — fires for configured pulse_ms
#define LISY_SOL_SET_PULSE_TIME         0x18  // payload: [solenoid_num, pulse_ms]
#define LISY_SOL_SET_RECYCLE_TIME       0x19  // payload: [solenoid_num, recycle_ms]
#define LISY_SOL_PULSE_WITH_PWM         0x1A  // not used

// --- Displays (no response, API >= 0.9) ---
// Command byte = 0x1E + display_index (displays 0..6)
// Payload: [byte_count, ...segment_bytes...]
// For 7-segment type 3: each byte encodes one digit in dp-g-f-e-d-a-b-c bit order
#define LISY_DISPLAY_0                  0x1E
#define LISY_DISPLAY_1                  0x1F
#define LISY_DISPLAY_2                  0x20
#define LISY_DISPLAY_3                  0x21
#define LISY_DISPLAY_4                  0x22
#define LISY_DISPLAY_5                  0x23
#define LISY_DISPLAY_6                  0x24

// --- Switches ---
#define LISY_SWITCH_GET_STATUS          0x28  // payload: [switch_num]
                                              // RPU responds: 0=inactive, 1=active, 2=doesn't exist
#define LISY_SWITCH_GET_CHANGED         0x29  // no payload
                                              // RPU responds: 0x7F if no changes
                                              //   otherwise 1 byte: bit7=state, bits0-6=switch_num

// --- Sound (not needed for original Trident chime board) ---
#define LISY_SOUND_PLAY                 0x32
#define LISY_SOUND_STOP_ALL             0x33
#define LISY_SOUND_PLAY_FILE            0x34
#define LISY_SOUND_TTS                  0x35
#define LISY_SOUND_SET_VOLUME           0x36

// --- Hardware rules (flipper logic offloaded to RPU) ---
#define LISY_HW_RULE_SOLENOID           0x3C  // 10-byte payload

// --- General ---
#define LISY_RESET                      0x64  // RPU responds: 0x00
#define LISY_WATCHDOG                   0x65  // RPU responds: 0x00  (called every 500ms by MPF)

// Display type codes (returned in response to LISY_INFO_GET_DISPLAY_DETAILS)
#define LISY_DISPLAY_TYPE_BCD           0x01
#define LISY_DISPLAY_TYPE_BCD_DP        0x02
#define LISY_DISPLAY_TYPE_7SEG          0x03  // Stern Trident uses this
#define LISY_DISPLAY_TYPE_14SEG         0x04
#define LISY_DISPLAY_TYPE_ASCII         0x05
#define LISY_DISPLAY_TYPE_ASCII_DP      0x06