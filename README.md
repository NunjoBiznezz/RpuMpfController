# RpuMpfController

Firmware for the [RPU (Remote Pinball Unit)](https://github.com/RetroPinUpgrade/RPU) board family that bridges classic Stern pinball machines to [Mission Pinball Framework (MPF)](https://missionpinball.org/) via the [LISY serial protocol](https://lisy.dev/).

The RPU board replaces the original 6800-family CPU in the pinball machine's CPU socket (rev 102) or plugs into the J5 connector (rev 3/4). This firmware runs on the ATmega2560 and translates MPF LISY commands into direct hardware writes to the PIA chips (U10/U11) that control lamps, displays, solenoids, and switches.

---

## Supported hardware

| Board token | `RPU_OS_HARDWARE_REV` | Physical form factor |
|---|---|---|
| `BOARD_RPU_V3` | 3 | MEGA2560 PRO → J5 connector |
| `BOARD_RPU_V4` | 4 | MEGA2560 PRO → J5 connector, with OLED/WIFI |
| `BOARD_RPU_V102` | 102 | MEGA2560 PRO → CPU socket interposer, with OLED/WIFI |

All three boards target `-17`, `-35`, MPU-100, and MPU-200 machines (`RPU_MPU_ARCHITECTURE = 1`).

---

## How configuration works

Configuration is split into three layers:

```
platformio.ini          ← two build flags per environment:
                            -DBOARD_RPU_V102  (board silicon)
                            -DMACHINE_STERN_TRIDENT  (machine peripherals)
                                   |
                    +--------------+---------------+
                    |                              |
          src/boards/rpu_v102.h         src/machines/stern_trident.h
          (three base defines)          (feature flags + resource counts)
                    |                              |
                    +--------------+---------------+
                                   |
                         include/RPU_config.h
                         (derives timing, digit counts, lamp banks, etc.)
```

`RPU_config.h` is a pure derivation layer — it reads the defines set by the board and machine headers and computes everything else. **Do not edit `RPU_config.h` to configure a target.**

### Board headers (`src/boards/`)

A board header defines only the three base hardware facts:

```cpp
#pragma once
#define RPU_OS_HARDWARE_REV    102
#define RPU_MPU_ARCHITECTURE   1
#define RPU_MPU_BUILD_FOR_6800 1
```

These are the only values that vary by board silicon. Everything else is either derived by `RPU_config.h` or belongs in the machine header.

### Machine headers (`src/machines/`)

A machine header defines:

1. **Feature flags** — which peripheral hardware the machine has:

   | Flag | Meaning |
   |---|---|
   | `RPU_OS_USE_DIP_SWITCHES` | Machine has DIP switches |
   | `RPU_OS_USE_SB100` | Machine uses the SB-100 sound board |
   | `RPU_OS_USE_SB300` | Machine uses the SB-300 sound board |
   | `RPU_OS_USE_WAV_TRIGGER_1p3` | Machine uses a WavTrigger 1.3 |
   | `RPU_OS_USE_7_DIGIT_DISPLAYS` | Machine wires 7-digit display boards |
   | `RPU_OS_USE_AUX_LAMPS` | Machine has auxiliary lamp boards (88 lamps) |

2. **Resource counts** — used in the LISY protocol handshake:

   ```cpp
   constexpr uint8_t MACHINE_NUM_SWITCHES   = 34;
   constexpr uint8_t MACHINE_NUM_SOLENOIDS  = 15;
   constexpr uint8_t MACHINE_NUM_LAMPS      = 60;
   constexpr uint8_t MACHINE_NUM_DISPLAYS   = 6;
   constexpr uint8_t MACHINE_DISPLAY_TYPE   = 3;   // LISY type 3 = 7-segment
   constexpr uint8_t MACHINE_DISPLAY_DIGITS = 6;
   ```

---

## Adding a new target

### New board revision

1. Create `src/boards/rpu_vN.h` with the three base defines.
2. Add a new `[revN_settings]` section in `platformio.ini`:
   ```ini
   [revN_settings]
   extends = common_settings
   platform = atmelavr
   board = megaatmega2560
   monitor_speed = 115200
   framework = arduino
   build_flags = -DBOARD_RPU_VN
   ```
3. Add a corresponding dispatch branch in `include/RPU_config.h`:
   ```cpp
   #elif defined(BOARD_RPU_VN)
   #  include "boards/rpu_vN.h"
   ```

### New machine

1. Create `src/machines/<machine_name>.h` with feature flags and resource counts (see machine header section above).
2. Add a new app section and environment in `platformio.ini`:
   ```ini
   [my_machine_app]
   build_src_filter =
   build_flags = -DMACHINE_MY_MACHINE

   [env:mpf_controller_my_machine_v102]
   extends = rev102_settings, my_machine_app
   build_flags =
      ${rev102_settings.build_flags}
      ${my_machine_app.build_flags}
   ```
3. Add a dispatch branch in `include/RPU_config.h`:
   ```cpp
   #elif defined(MACHINE_MY_MACHINE)
   #  include "machines/my_machine.h"
   ```

---

## Building

```sh
# Build all environments
pio run

# Build and upload one environment
pio run -e mpf_controller_v102 --target upload

# Monitor serial output
pio device monitor -e mega_hello
```

---

## Project structure

```
include/
  RPU.h               Public API
  RPU_config.h        Derivation layer — reads board+machine, computes constants
  lisy_protocol.h     LISY command byte constants

src/
  main.cpp            LISY serial dispatch loop
  RPU.cpp             Hardware init, Timer1 ISR, PIA access
  boards/
    rpu_v3.h          Board silicon defines for rev 3
    rpu_v4.h          Board silicon defines for rev 4
    rpu_v102.h        Board silicon defines for rev 102
  machines/
    stern_trident.h   Feature flags + resource counts for Stern Trident (1979)
  DisplayManager.*    Display multiplexing
  LampManager.*       Lamp multiplexing
  SwitchManager.*     Switch debounce and reporting
  SolenoidManager.*   Solenoid firing
  (other managers)

specs/
  original/           Goal-level specs for each subsystem or feature area
```

---

## LISY protocol

MPF communicates at 115200 baud 8N1. MPF sends a command byte; this firmware responds. The protocol is LISY type 3 (7-segment displays). See `include/lisy_protocol.h` for command constants and `specs/original/` for subsystem specs.
