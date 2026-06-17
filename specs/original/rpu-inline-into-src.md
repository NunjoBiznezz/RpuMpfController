# Spec: Move RPU library into src/include

**Status:** Draft — awaiting sign-off  
**Date:** 2026-06-17  
**Depends on:** milestone-1-attract-mode.md, multi-machine-config-headers.md (both approved)

---

## Goal

The RPU code is the core of this application, not a reusable third-party library.
Move it out of `lib/RPU/` and into the project's first-class `src/` and `include/`
directories so it is compiled as part of the application rather than as a separate
library unit.

---

## File moves

| From | To |
|---|---|
| `lib/RPU/include/RPU.h` | `include/RPU.h` |
| `lib/RPU/include/RPU_config.h` | `include/RPU_config.h` |
| `lib/RPU/src/RPU_Internals.h` | `include/RPU_Internals.h` |
| `lib/RPU/src/CircularStack.h` | `include/CircularStack.h` |
| `lib/RPU/src/TimedStack.h` | `include/TimedStack.h` |
| `lib/RPU/src/DisplayManager.h` | `include/DisplayManager.h` |
| `lib/RPU/src/LampManager.h` | `include/LampManager.h` |
| `lib/RPU/src/SolenoidManager.h` | `include/SolenoidManager.h` |
| `lib/RPU/src/SwitchManager.h` | `include/SwitchManager.h` |
| `lib/RPU/src/Dash51.h` | `include/Dash51.h` |
| `lib/RPU/src/RPU.cpp` | `src/RPU.cpp` |
| `lib/RPU/src/DisplayManager.cpp` | `src/DisplayManager.cpp` |
| `lib/RPU/src/LampManager.cpp` | `src/LampManager.cpp` |
| `lib/RPU/src/SolenoidManager.cpp` | `src/SolenoidManager.cpp` |
| `lib/RPU/src/SwitchManager.cpp` | `src/SwitchManager.cpp` |
| `lib/RPU/src/Dash51.cpp` | `src/Dash51.cpp` |
| `lib/RPU/src/DipSwitches.cpp` | `src/DipSwitches.cpp` |
| `lib/RPU/src/SB100Sound.cpp` | `src/SB100Sound.cpp` |
| `lib/RPU/src/SB300.cpp` | `src/SB300.cpp` |
| `lib/RPU/src/SqualkAndTalk.cpp` | `src/SqualkAndTalk.cpp` |
| `lib/RPU/src/WType11Sound.cpp` | `src/WType11Sound.cpp` |
| `lib/RPU/src/WTypeSound12.cpp` | `src/WTypeSound12.cpp` |

After the move, `lib/RPU/` and `lib/RPU/library.json` are deleted.

---

## Include path fixes

All `.cpp` files currently include internal headers with bare names
(e.g. `#include "LampManager.h"`). PlatformIO resolves these because `lib/RPU/src/`
is on the library's include path. After the move, `include/` is on the global
include path, so bare-name includes will resolve correctly without changes.

The two files that include `"RPU.h"` (the public header) from inside the library
will continue to work because `include/` is already on the project include path.

No `#include` directives need to change.

---

## platformio.ini changes

- Remove `lib_deps = RPU` from `[mpf_controller_app]`.
- No other changes. The `[common_settings]` / env sections are unaffected.

---

## Acceptance criteria

1. `pio run` builds all 4 environments cleanly with no include errors.
2. `lib/RPU/` directory no longer exists.
3. `lib_deps = RPU` is removed from `platformio.ini`.
4. No `#include` directives are changed in any source file.
