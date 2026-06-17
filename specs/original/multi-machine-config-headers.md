# Spec: Multi-machine config headers (Option A)

**Status:** Draft — awaiting sign-off  
**Date:** 2026-06-17  
**Depends on:** milestone-1-attract-mode.md (approved)

---

## Goal

Reorganize the machine-specific constants out of `platformio.ini` build flags and
into per-machine C++ header files. The LISY protocol dispatch in `main.cpp` stays
unchanged. Adding a new machine requires only a new header and a new PlatformIO
environment — no changes to shared source.

---

## What changes

### New file: `src/machines/<machine>.h`

One header per machine. Trident is the first. Each header defines the same set
of constants using `#pragma once` and a guard so it is safe to include from any
translation unit.

**`src/machines/stern_trident.h`** (initial content):

```cpp
#pragma once

// Stern Trident (1979) — machine resource counts for LISY protocol handshake.
// Source: MissionPinball/machine_files/stern_trident.yaml

constexpr uint8_t MACHINE_NUM_SWITCHES  = 34;
constexpr uint8_t MACHINE_NUM_SOLENOIDS = 15;
constexpr uint8_t MACHINE_NUM_LAMPS     = 60;
constexpr uint8_t MACHINE_NUM_DISPLAYS  = 6;
constexpr uint8_t MACHINE_DISPLAY_TYPE  = 3;  // LISY type 3 = 7-segment
constexpr uint8_t MACHINE_DISPLAY_DIGITS = 6;
```

### Changed: `src/main.cpp`

Remove `using` of the -D macro constants. Replace with:

```cpp
#include MACHINE_CONFIG_HEADER
```

where `MACHINE_CONFIG_HEADER` is a string token supplied by the build system.

### Changed: `platformio.ini`

Remove the six `-DMACHINE_*` build flags from `[mpf_controller_app]`.
Add `-DMACHINE_CONFIG_HEADER=` to each machine environment instead.

Each `[env:mpf_controller_*]` gets one additional flag:

```ini
-DMACHINE_CONFIG_HEADER="machines/stern_trident.h"
```

`[mpf_controller_app]` retains shared flags (none currently) and `lib_deps`.

---

## File layout after change

```
src/
  main.cpp                        ← unchanged except for the header include
  machines/
    stern_trident.h               ← new
platformio.ini                    ← updated envs
specs/
  original/
    multi-machine-config-headers.md
```

---

## Acceptance criteria

1. `pio run` builds all 4 environments cleanly.
2. Machine constants (`MACHINE_NUM_LAMPS`, etc.) resolve correctly from the
   header, not from `-D` flags.
3. Removing the `-DMACHINE_CONFIG_HEADER` flag from an environment produces a
   clear compile error (missing include), not a silent wrong value.
4. Adding a second machine requires only: a new `src/machines/<name>.h` and a
   new `[env:]` section in `platformio.ini` — no changes to `main.cpp` or
   `lib/RPU/`.

---

## Out of scope

- Machine-specific behavior differences (e.g., different startup sequences,
  display layouts). Those are a future concern if machines diverge beyond counts.
- Removing the `mega_hello` environment or restructuring it.
