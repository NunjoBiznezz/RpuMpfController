# RpuMPFController — Architecture & Protocol Reference

## What this project is

Firmware for the RPU v102 board that allows Mission Pinball Framework (MPF)
running on a Raspberry Pi to control a Stern Trident (1979) pinball machine
using the original hardware.

The RPU v102 board replaces the original MPU board and connects to the
machine's existing wiring harness (switches, solenoids, lamps, displays).
This firmware makes the RPU speak the LISY serial protocol so MPF can
control the machine without knowing anything about the original hardware.

---

## System architecture

```
[ Stern Trident hardware ]
  switches, solenoids, lamps, segment displays
         |
    [ RPU v102 board ]  ← this firmware
         | USB serial (115200 baud, LISY protocol)
    [ Raspberry Pi ]
         | Python
    [ MPF (Mission Pinball Framework) ]
         | BCP (Ball Control Protocol, TCP)
    [ GMC / Godot ]  ← development only, not on Pi
```

- **MPF** is the rules engine. It knows nothing about hardware specifics —
  it sends LISY commands and receives switch events.
- **This firmware** translates LISY commands to real hardware signals and
  sends switch state changes back to MPF.
- **GMC** is a Mac-only development UI; it is not present on the Pi in
  production.

The MPF project lives at: `../MissionPinball`

---

## Development vs production

| | Mac (development) | Raspberry Pi (production) |
|---|---|---|
| Rules engine | MPF with `smart_virtual` platform | MPF with `lisy` platform |
| Hardware | None — simulated by MPF | RPU v102 via this firmware |
| Display | GMC (Godot) over BCP | LISY display commands to RPU |
| Config | `config/config_dev.yaml` | `config/config_rpi.yaml` |

---

## LISY protocol

All communication is MPF-initiated over serial (115200 baud, 8N1).
The RPU never sends unsolicited data.

Command byte definitions are in `include/lisy_protocol.h`.

### Startup sequence (MPF calls these in order on connect)

1. **Reset** (0x64) → respond `0x00`
2. **GetHardware** (0x00) → respond null-terminated string, e.g. `"LISY80\0"`
3. **GetLisyVersion** (0x01) → respond null-terminated string, e.g. `"4.02\0"`
4. **GetApiVersion** (0x02) → respond null-terminated string, e.g. `"0.9\0"`
5. **GetNumLamps** (0x03) → respond 1 byte
6. **GetNumSolenoids** (0x04) → respond 1 byte
7. **GetNumDisplays** (0x06) → respond 1 byte
8. **GetSwitchCount** (0x09) → respond 1 byte
9. **GetModernLightsCount** (0x13) → respond `0x00` (no RGB LEDs on Trident)
10. **GetStatusOfSwitch** (0x28) × N → respond `0`=inactive, `1`=active, `2`=not present
11. **GetDisplayDetails** (0x07) × N → respond 2 bytes `[type, digit_count]`
    - Trident displays: type `3` (7-segment), 6 digits each

### Runtime switch polling

MPF polls continuously:

- **GetChangedSwitches** (0x29) → respond `0x7F` if no changes,
  otherwise 1 byte: bit 7 = state, bits 0–6 = switch number

This is a tight loop. The RPU must respond with minimal latency.

### Lamp control (no response)

- **LampOn** (0x0B) `[lamp_num]`
- **LampOff** (0x0C) `[lamp_num]`

### Solenoid control (no response)

- **SetPulseTime** (0x18) `[solenoid_num, pulse_ms]`
- **Pulse** (0x17) `[solenoid_num]`
- **SolenoidOn** (0x15) `[solenoid_num]`
- **SolenoidOff** (0x16) `[solenoid_num]`
- **SetRecycleTime** (0x19) `[solenoid_num, recycle_ms]`
- **HardwareRule** (0x3C) 10-byte payload (flipper logic)

### Display control (no response, API >= 0.9)

Command byte = `30 + display_index`. Payload: `[byte_count, ...segment_bytes]`.

For 7-segment type 3, each byte encodes one digit:
`bit7=dp, bit6=g, bit5=f, bit4=e, bit3=d, bit2=a, bit1=b, bit0=c`

MPF sends pre-encoded bytes; the RPU drives the display hardware directly.

### Watchdog (every 500ms)

- **Watchdog** (0x65) → respond `0x00`

If watchdog pings stop arriving, MPF has likely crashed. The RPU should
disable all solenoids to safe the machine.

---

## Stern Trident hardware mapping

Switch, solenoid, and lamp numbers used in the LISY protocol must match
the numbers assigned in the MPF machine file at:
`../MissionPinball/machine_files/stern_trident.yaml`

That file is the authoritative source for numbering. Pin mapping to the
RPU v102 hardware headers is to be established when the RPU board code
is integrated.

### Counts (from machine file)

| Resource | Count |
|---|---|
| Switches | ~36 |
| Solenoids | ~15 |
| Lamps | ~60 |
| Displays | 6 (4 scores + ball + credits), 6-digit 7-segment |

---

## What is not in scope for this firmware

- Game rules — those live entirely in MPF (`../MissionPinball`)
- Sound — the original Trident has a hardware chime board; MPF can
  trigger it via a solenoid driver if needed, but no LISY sound commands
  are required
- Display rendering logic — MPF sends pre-encoded segment bytes; the
  firmware just forwards them to the hardware
