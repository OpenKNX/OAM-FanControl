# OAM-FanControl

OpenKNX application for decentralised ventilation fans (Maico PPB30, Fawas HST,
ebm-papst AxiRev) on RP2040.

> **Rewritten from scratch on branch `dev` (2026-08-15).** The old design — fan-type classes,
> operating-mode enumeration, per-direction PWM pins — is gone. `references/` holds the binding
> specification; read it before changing behaviour.

## Where the truth lives

| Document | Content |
|---|---|
| `references/Anforderungen_Dezentrale_Lueftersteuerung.md` | Original concept paper, bus-agnostic. Frozen, not maintained |
| `references/Review_Anforderungen_KNX-Sicht.md` | **Where the software deviates from it.** Section 5 holds the 7 unresolved deviations, section 6 the verified layout |
| `../OFM-FanControl/doc/Applikationsbeschreibung-Fan.md` | User documentation **and** source of the ETS context help |
| `../../FirmwareEntwicklung_TAS_UP/TAS-UP-4x-TouchRGB/doc/OpenKNX-ETS-XML-Styleguide.md` | OpenKNX ETS-XML conventions |

## The one thing to understand first

**The fans are driven bipolar on a single output.** Duty cycle carries speed *and* direction:

| Duty at the fan | Meaning |
|---|---|
| 0 % | full speed direction A |
| mid position (parameter, default 50 %) | **standstill** |
| 100 % | full speed direction B |

So **0 % is not "off"** — it is full speed one way. The safe state is the mid position. Every
place that stops a fan writes the mid position, never 0. A mid position of 0 % configures an
ordinary non-reversing fan (0 = off, 100 = full).

A Maico contains two fans that always turn together; they hang on one output. The MrSpieb board
exposes a second, **mirrored** output per node carrying the identical signal — that is not a
second direction.

## Build & toolchain

PlatformIO CLI is not in PATH. Claude Code uses bash, so `$USERPROFILE`.

```bash
# Reg1 Fan-Addon-X2 (2 nodes, tacho, dual core)
"$USERPROFILE/.platformio/penv/Scripts/pio.exe" run -e develop_RP2040

# MrSpieb HW-FanControl (mirrored outputs, no tacho, single core)
"$USERPROFILE/.platformio/penv/Scripts/pio.exe" run -e develop_RP2040_MrSpieb
```

`lib/OGM-Common/scripts/build/OpenKNX-Build.ps1 -env <env>` wraps pio; it can return exit 1 on
the first run of a new environment even when the build succeeded — trust the `[SUCCESS]` line.

### OpenKNXproducer (4.3.12, at `%USERPROFILE%/bin/`)

```powershell
# knxprod + knxprod.h
& (Join-Path $env:USERPROFILE "bin/OpenKNXproducer.exe") create --Debug -h include/knxprod.h src/Fan.xml

# context help, generated from the application description — run after editing that file
cd ..\OFM-FanControl
& (Join-Path $env:USERPROFILE "bin/OpenKNXproducer.exe") baggages -b "src\Baggages\Help_de" -d "doc\Applikationsbeschreibung-Fan.md" -p FAN
```

`WARN 003: Function with name newline was never called` comes from OGM-Common's own script and
is harmless. Never edit `include/knxprod.h`.

### Producer gotchas learned the hard way

- **Unknown `op:define` attributes are ignored silently.** `KoBlockSize="32"` did nothing and
  produced no warning.
- **The KO block size is the highest `%Kn%` used, plus one.** There is no attribute for it
  (`doc/Anleitung-OpenKNXproducer.md`, section KoOffset). Reserve therefore means **gaps inside
  the block**, never room at the top.
- **`ReplacesVersions` is mandatory.** Empty or absent is rejected. To cut the migration path,
  list only the application's own version.
- **`HelpContext="%DOC%"` derives the file name from the parameter's `Text=`** (ä→ae, ö→oe,
  ü→ue, ß→ss, spaces→hyphens) and requires that file to exist. Vague labels give useless help
  file names — that is why parameters are called "Anlaufpuls Dauer" and not "Dauer".
- **Read-on-Init flags do not work** and are stripped. Startup values come from cyclic sending.

## Dependency restore — never run it yourself

`restore/Restore-Dependencies.ps1` needs an elevated shell (`mklink /D`) and recreates the
symlinks in `lib/`. **Claude Code must not run it and must not touch the symlinks.** Declare a
new dependency by adding a line to `dependencies.txt`, then ask the user to run:

```powershell
cd restore
.\Restore-Dependencies.ps1
```

## Frozen layout — do not change after the first release

| | Value |
|---|---|
| KOs per channel | **32** (23 used, free: 8, 9, 20–26) |
| Parameters per channel | **80 byte** (used up to 61, reserve 62–79) |
| Channels | 2 — channel 1 = KO 20–51, channel 2 = KO 52–83 |
| Module-wide parameter | PWM frequency at absolute offset 114 |

A new KO belongs in one of the gaps. Anything above 28 grows the block and shifts every KO
number of channel 2, which destroys the group address links of existing ETS projects. The same
applies to parameter offsets.

Application id: `OpenKnxId 0xAF`, `ApplicationNumber 0x86`, version `0.1`, `ReplacesVersions 0.1`.
A dedicated developer ApplicationNumber is still to be negotiated and must be switched before
any external test.

## Source layout

```
src/main.cpp                     Entry point, module registration (Fan=1, Logic=2)
src/Fan.xml                      Producer entry point, op:define per module
src/Fan.conf.xml                 Versions
include/hardware.h               Board selector via DEVICE_* define
include/Reg1_FanAddon_X2.h       One output per node, tacho, PWM inverted
include/MrSpiebFanControlHardware.h  Mirrored outputs, no tacho, PWM not inverted

../OFM-FanControl/src/
  FanTypes.h                     Enums: channel type, setpoint source, direction mode, faults
  IFanHardware.h                 drive(direction, speed) / stop() / setMidpoint()
  RP2040FanHardware.*            PWM incl. mirror output and board polarity
  FanChannel.*                   The node: state machine, controller, curve, publishing
  FanModule.*                    Channels, KO routing, flash, core-1 tacho, startup delay
  TachoReader.*                  Pulse counting on core 1
  Fan.share.xml / Fan.templ.xml  ETS application
```

## Firmware conventions

- **State machine** per channel: `Off → StartPulse → Running`, plus `DeadTime` on a direction
  change. Stopping always writes the mid position and opens the load switch.
- **Board differences belong in the board header**, never in ETS. Polarity
  (`FAN_PWM_ACTIVE_LOW`), mirror output (`FANx_S2_PWM_PIN`) and tacho pins are compile-time.
  The ETS application is byte-identical for both boards.
- **Persistent state** (enable latch, suspended, operating seconds) is written immediately on
  change via `openknx.flash.save(true)`, not left to the periodic save.
- **The enable latch starts unlocked** on fresh flash and the watchdog only starts after the
  first Freigabe telegram — otherwise an installation that never wires Freigabe would never run.
- Module registration order fixes the flash regions. **FanControl stays at id 1.**
- `DPT 13.002` has 0.0001 m³/h resolution — the raw value is m³/h × 10000.

## Logic module

Linked and building since 2026-08-15 (v4.4.1, 30 channels, KoOffset 280, ~5 % extra flash).

Two things that cost time and are not guessable:

- Its `op:define` needs `ModuleType="10"` — **the same type as BASE**. The module registers its
  version information under the BASE type; every other value makes the producer complain
  "ModuleType for LOG (30) is not the same as BASE (10)".
- `op:verify ModuleVersion` must track `lib/OFM-LogicModule/library.json` (currently `4.4`).
  A stale value fails the "Minimal required module versions" check and the producer then skips
  writing the .knxprod — while still writing `knxprod.h`, so the next build fails with a
  confusing cascade of "`PT_LogicDpt` has not been declared" errors. If you see those, re-run
  the producer first and read *its* output, not the compiler's.

## Restore script hazard

The restore/update script checks every dependency out **detached at the pinned commit** — and
`lib/OFM-FanControl` is one of those dependencies. Our own development lives on branch `dev`
in that repo, so after every restore run the module sits on a detached HEAD. Uncommitted work
survives (the tree is untouched), but commit anything and it lands nowhere.

After running the restore script:

```bash
cd ../OFM-FanControl && git checkout dev
```

## Known open points

- Nothing has been tested on hardware: PWM polarity, load-switch direction, tacho counting.
- `../OFM-FanControl/test/test_fan_logic.cpp` still tests the deleted `Fan` class and no host
  compiler exists on this machine, so `pio test -e native` cannot run here at all.
- **7 deviations from the requirements are unresolved** — `references/Review_Anforderungen_KNX-Sicht.md`
  section 5. Four have functional consequences; the enable watchdog not starting after a restart
  (5.1) is the safety-relevant one.
