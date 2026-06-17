# Spec: Board/machine config split

**Status:** Approved — implemented  
**Date:** 2026-06-17  
**Depends on:** rpu-inline-into-src.md (approved)

---

## Goal

Every define in the current build that cannot be inferred solely from
`RPU_OS_HARDWARE_REV + RPU_MPU_ARCHITECTURE + RPU_MPU_BUILD_FOR_6800` moves
into a machine header. The three base defines move into a board header.
`RPU_config.h` becomes a pure derivation layer. `platformio.ini` drops to
two meaningful flags per environment: one board token, one machine token.

---

## Analysis: what can and cannot be derived from the three base flags

### `RPU_MPU_ARCHITECTURE == 1` block in RPU_config.h

| Constant | Board-derivable? | Notes |
|---|---|---|
| `RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL` | ✓ | Pure timing for arch-1 |
| `CONTSOL_DISABLE_FLIPPERS/COIN_LOCKOUT` | ✓ | Fixed bit masks |
| `RPU_OS_SWITCH_DELAY_IN_MICROSECONDS` | ✓ | Board timing |
| `RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS` | ✓ | Board timing |
| `NUM_SWITCH_BYTES` / `MAX_NUM_SWITCHES` | ✓ | Fixed capacity |
| `RPU_OS_MASK_SHIFT_1/2` | ✗ | Depends on `RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS` |
| `RPU_OS_NUM_DIGITS`, `RPU_OS_MAX_DISPLAY_SCORE`, `RPU_OS_ALL_DIGITS_MASK` | ✗ | Depends on `RPU_OS_USE_7_DIGIT_DISPLAYS` |
| `RPU_NUM_LAMP_BANKS`, `RPU_MAX_LAMPS` | ✗ | Depends on `RPU_OS_USE_AUX_LAMPS` |

### `RPU_MPU_ARCHITECTURE > 9` block

All constants in this block (`RPU_MAX_LAMPS`, `RPU_OS_NUM_DIGITS`, etc.) are fully
determined by the architecture number alone — no machine flags involved. No changes
needed to this block.

### Feature flags currently in platformio.ini rev_settings

These describe machine peripherals, not board silicon. They move to machine headers:

| Flag | Machine-specific reason |
|---|---|
| `RPU_OS_USE_DIP_SWITCHES` | Does this machine have DIP switches? |
| `RPU_OS_USE_SB100` | Does this machine use the SB-100 sound board? |
| `RPU_OS_USE_WAV_TRIGGER_1p3` | Does this machine use a WavTrigger? |
| `RPU_OS_USE_7_DIGIT_DISPLAYS` | Does this machine wire 7-digit display boards? |
| `RPU_OS_USE_AUX_LAMPS` | Does this machine have auxiliary lamp boards? |
| `RPU_OS_USE_SB300` | Does this machine use the SB-300 sound board? |

---

## Changes

### New: `src/boards/rpu_v3.h`
```cpp
#pragma once
#define RPU_OS_HARDWARE_REV    3
#define RPU_MPU_ARCHITECTURE   1
#define RPU_MPU_BUILD_FOR_6800 1
```

### New: `src/boards/rpu_v4.h`
```cpp
#pragma once
#define RPU_OS_HARDWARE_REV    4
#define RPU_MPU_ARCHITECTURE   1
#define RPU_MPU_BUILD_FOR_6800 1
```

### New: `src/boards/rpu_v102.h`
```cpp
#pragma once
#define RPU_OS_HARDWARE_REV    102
#define RPU_MPU_ARCHITECTURE   1
#define RPU_MPU_BUILD_FOR_6800 1
```

### Updated: `src/machines/stern_trident.h`

Add machine feature flag defines above the existing `constexpr` counts:

```cpp
// Machine peripheral flags
#define RPU_OS_USE_DIP_SWITCHES
#define RPU_OS_USE_SB100
// Trident uses standard 6-digit displays (no RPU_OS_USE_7_DIGIT_DISPLAYS)
// Trident uses standard 60-lamp board (no RPU_OS_USE_AUX_LAMPS)
```

### Updated: `include/RPU.h`

Insert board and machine dispatch at the very top (before `#include "RPU_config.h"`):

```cpp
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
```

Because `RPU.h` is included by every translation unit (main.cpp, RPU.cpp, all managers),
both headers are guaranteed to be visible everywhere they are needed.

### Updated: `src/main.cpp`

Remove the machine dispatch block that was added in the multi-machine-config-headers
spec — it is now handled by `RPU.h`:

```cpp
// Remove:
#if defined(MACHINE_STERN_TRIDENT)
#  include "machines/stern_trident.h"
#else
#  error "..."
#endif
```

### Updated: `include/RPU_config.h`

Update the three `#error` messages to tell the user to set a `BOARD_*` token instead
of editing `platformio.ini` directly. No structural changes to the derivation logic.

### Updated: `platformio.ini`

`[rev3_settings]`, `[rev4_settings]`, `[rev102_settings]`: replace all `RPU_OS_*`
build flags with a single board token. Non-flag keys (`platform`, `board`,
`monitor_speed`, `framework`) are unchanged.

```ini
[rev3_settings]
extends = common_settings
platform = atmelavr
board = megaatmega2560
framework = arduino
monitor_speed = 115200
build_flags = -DBOARD_RPU_V3

[rev4_settings]
...
build_flags = -DBOARD_RPU_V4

[rev102_settings]
...
build_flags = -DBOARD_RPU_V102
```

`[mpf_controller_app]` already supplies `-DMACHINE_STERN_TRIDENT` and is unchanged.

`[env:mega_hello]` already includes `${mpf_controller_app.build_flags}` (machine token)
and `${rev102_settings.build_flags}` (board token) — no change needed there either.

---

## Include order guarantee

After this change, every `.cpp` file that does `#include "RPU.h"` will process:

1. `boards/rpu_v102.h` → defines `RPU_OS_HARDWARE_REV`, `RPU_MPU_ARCHITECTURE`, `RPU_MPU_BUILD_FOR_6800`
2. `machines/stern_trident.h` → defines `RPU_OS_USE_DIP_SWITCHES`, `RPU_OS_USE_SB100`, machine counts
3. `RPU_config.h` → validates both, derives `RPU_OS_NUM_DIGITS`, `RPU_MAX_LAMPS`, etc.

No `-D` flags carrying RPU logic remain in `platformio.ini`.

---

## Acceptance criteria

1. `pio run` builds all 4 environments cleanly.
2. `platformio.ini` rev_settings sections contain only a single `-DBOARD_*` build flag.
3. No `RPU_OS_*` defines appear in `platformio.ini`.
4. Adding a new machine requires only: a new `src/machines/<name>.h` and `-DMACHINE_<NAME>` in the new env.
5. Adding a new board revision requires only: a new `src/boards/rpu_vN.h` and `-DBOARD_RPU_VN` in the new env.
