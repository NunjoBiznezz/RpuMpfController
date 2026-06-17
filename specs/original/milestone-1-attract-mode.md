# Spec: Milestone 1 — Attract Mode LISY Bridge

**Status:** Approved — implemented  
**Target environment:** `env:mpf_controller_v102` (RPU v102, Stern Trident)  
**MPF version:** 0.80  
**Date:** 2026-06-17

---

## Goal

MPF connects over serial, completes the LISY startup handshake, then runs an
attract mode light show by sending lamp and display commands. The firmware
faithfully executes those commands and drives the Trident hardware.

This milestone does **not** require switches, solenoids, or watchdog safety
logic to function — those are stubbed.

---

## In scope

| Feature | Detail |
|---|---|
| LISY startup handshake | All 11 startup commands handled (see below) |
| Lamp control | `LampOn` / `LampOff` drive physical lamp hardware |
| Display control | Display commands (type 3, 7-segment) drive physical display hardware |
| Switch stub | `GetChangedSwitches` always returns `0x7F`; `GetStatusOfSwitch` always returns `2` (not present) |
| Watchdog stub | Responds `0x00`; no timeout action |
| Serial framing | 115200 baud, 8N1, MPF-initiated request/response |

---

## Out of scope (deferred to later milestones)

- Real switch polling and debounce
- Solenoid control (pulse, hold, hardware rules, flipper logic)
- Watchdog timeout safety action (disabling solenoids)
- Sound / chime board control
- RGB LED support

---

## LISY startup sequence

MPF sends these commands in order on connect. The firmware must respond
correctly to each before MPF proceeds.

| # | Command | Byte | Response |
|---|---|---|---|
| 1 | Reset | `0x64` | `0x00` |
| 2 | GetHardware | `0x00` | `"LISY80\0"` |
| 3 | GetLisyVersion | `0x01` | `"4.02\0"` |
| 4 | GetApiVersion | `0x02` | `"0.9\0"` |
| 5 | GetNumLamps | `0x03` | `MACHINE_NUM_LAMPS` (1 byte) |
| 6 | GetNumSolenoids | `0x04` | `MACHINE_NUM_SOLENOIDS` (1 byte) |
| 7 | GetNumDisplays | `0x06` | `MACHINE_NUM_DISPLAYS` (1 byte) |
| 8 | GetSwitchCount | `0x09` | `MACHINE_NUM_SWITCHES` (1 byte) |
| 9 | GetModernLightsCount | `0x13` | `0x00` (no RGB LEDs) |
| 10 | GetStatusOfSwitch | `0x28` + switch# | `2` (not present) — repeated N times |
| 11 | GetDisplayDetails | `0x07` + display# | `[3, 6]` (type 3, 6 digits) — repeated N times |

---

## Runtime command handling

### Switch polling (stub)

| Command | Byte | Response |
|---|---|---|
| GetChangedSwitches | `0x29` | `0x7F` (no changes) |
| GetStatusOfSwitch | `0x28` + switch# | `2` (not present) |

### Lamp control

| Command | Byte | Payload | Response |
|---|---|---|---|
| LampOn | `0x0B` | `[lamp_num]` | none |
| LampOff | `0x0C` | `[lamp_num]` | none |

Lamp numbers map to `RPU::LampManager`. The existing `LampManager` API
(`setLampState`) is used to set lamp state; the ISR-driven multiplex loop
handles physical output.

### Display control

Command byte = `30 + display_index`. Payload: `[byte_count, ...segment_bytes]`.

Each segment byte encodes one digit:
`bit7=dp, bit6=g, bit5=f, bit4=e, bit3=d, bit2=a, bit1=b, bit0=c`

MPF sends pre-encoded bytes. The firmware forwards them to `RPU::DisplayManager`.

### Watchdog (stub)

| Command | Byte | Response |
|---|---|---|
| Watchdog | `0x65` | `0x00` |

No timeout tracking. Full watchdog safety logic is deferred.

---

## Machine constants (Stern Trident)

Place these as `-D` build flags in the `[common_settings]` section of
`platformio.ini`.

> **Note:** These values are Stern Trident defaults. When support for other
> machines is added, extract these into per-machine environment sections.

| Constant | Value | Source |
|---|---|---|
| `MACHINE_NUM_SWITCHES` | `34` | `stern_trident.yaml` |
| `MACHINE_NUM_SOLENOIDS` | `15` | `stern_trident.yaml` |
| `MACHINE_NUM_LAMPS` | `60` | `stern_trident.yaml` |
| `MACHINE_NUM_DISPLAYS` | `6` | Architecture doc (4 score + ball + credits) |
| `MACHINE_DISPLAY_TYPE` | `3` | 7-segment |
| `MACHINE_DISPLAY_DIGITS` | `6` | 6 digits per display |

---

## Implementation touchpoints

| File | Change |
|---|---|
| `platformio.ini` | Add machine constant `-D` flags to `[common_settings]` |
| `src/main.cpp` | Implement `setup()` / `loop()` with serial command dispatch |
| `include/lisy_protocol.h` | Command byte constants (already exists as stub) |
| `lib/RPU/` | Wire `LampManager` and `DisplayManager` into command handlers |

No new files should be needed beyond `src/main.cpp` growing a command dispatch
loop and `lisy_protocol.h` being populated.

---

## Acceptance criteria

1. `pio run -e mpf_controller_v102` compiles cleanly.
2. MPF 0.80 with `lisy` platform connects and completes the startup handshake without errors.
3. MPF's attract mode light show commands cause the Trident's lamps to respond visibly.
4. MPF's display commands cause the Trident's 7-segment displays to show the correct digits.
5. MPF does not disconnect or error due to switch or watchdog responses.
