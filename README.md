# OpenKNX FanControl

KNX application for decentralised ventilation with reversing fans. One device drives two fan
nodes; several nodes form a group over shared group addresses, coordinated by one master.

> **Status: development.** Nothing has been verified on hardware yet. The KO and parameter
> layout is frozen but the application number is still provisional.

## Concept

A node is a fan plus its drive electronics, and one ETS channel. Exactly one node in a group is
the **master**: it produces the power setpoint, sets the reversing tact and sends a keep-alive.
Every other node is a **slave** that applies the group setpoint with its own limits and share
factor. Both roles run the same firmware and are distinguished by a single parameter.

Direction is never commanded directly. Each node derives its own airflow direction from its
phase assignment and the master's tact, so half the group runs supply air while the other half
runs exhaust air, and they swap on every cycle.

![Group topology: one master and three slaves on shared group addresses, phase and counter-phase
moving air in opposite directions](references/%C3%9Cbersicht.drawio.png)

*Diagram in German. Source: [`references/Übersicht.drawio`](references/Übersicht.drawio).*

## Features

- **Two channel types** — reversible (two airflow directions, tact) and non-reversible.
- **Bipolar drive** for reversing fans: speed and direction on a single output. 0 % is full
  speed in direction A, the midpoint is standstill, 100 % is full speed in direction B. The
  midpoint is the safe state. Non-reversible fans are driven conventionally.
- **Setpoint sources** — fixed value, external communication object, or an internal
  P controller on CO₂ or relative humidity.
- **Per-node scaling** — share factor plus minimum and maximum drive level per direction.
- **Start pulse** to break static friction, and a dead time before every direction change.
- **Boost ventilation** as a master function, with its own runtime and power level.
- **Two-stage monitoring** — a self-holding, power-fail-persistent enable latch, and a master
  watchdog that holds the last state until it expires.
- **Feedback** — speed, volume flow via a configurable curve (signed: positive is supply air),
  running state, direction, operating hours, fault and fault code.
- **Blockage detection** — two consecutive 5 s windows without tacho pulses while the fan is
  commanded to run.
- **Logic module** included with 30 channels.

KNX is not a safety bus. Interlocking with a fireplace additionally requires a hard-wired,
potential-free contact.

## Documentation

- [`../OFM-FanControl/doc/Applikationsbeschreibung-Fan.md`](https://github.com/cad435/OFM-FanControl/blob/dev/doc/Applikationsbeschreibung-Fan.md)
  — user documentation, parameter by parameter. Also the source of the ETS context help.
  Start here.
- [`CLAUDE.md`](CLAUDE.md) — developer reference: build commands, frozen layout, the gotchas
  that cost time.

The two documents in [`references/`](references/) are a pair, both in German:

- [`Anforderungen_Dezentrale_Lueftersteuerung.md`](references/Anforderungen_Dezentrale_Lueftersteuerung.md)
  — the original concept paper, written from the perspective of the trades that build and run
  the system, deliberately without any knowledge of the KNX bus.
- [`Review_Anforderungen_KNX-Sicht.md`](references/Review_Anforderungen_KNX-Sicht.md)
  — **where the software deviates from it, and why.** Includes the deviations that are still
  open, four of which have functional consequences.

## Hardware

Two boards are supported. Which one is built is a compile-time decision, not an ETS option —
PWM polarity and the number of outputs differ. The ETS application is identical for both.

| | OpenKNX Reg1 Fan-Addon-X2 | [MrSpieb HW-FanControl](https://github.com/mrspieb/HW-FanController) |
|---|---|---|
| Outputs per node | one | two, mirrored (identical signal, one fan each) |
| Tacho input | yes, opto-coupled | no |
| PWM polarity | inverted (level shifter driving an NMOS with pull-up) | not inverted |
| Build environment | `develop_RP2040` | `develop_RP2040_MrSpieb` |

Tested fan types: Maico PPB series and Fawas Air Solitaire 160 (ebm-papst AxiRev 126). Any
reversing fan that takes speed and direction on one PWM input should work.

MCU is an RP2040. The KNX transceiver is an NCN5120.

## Building

See [`CLAUDE.md`](CLAUDE.md) for the full command reference. Dependencies are symlinked into
`lib/` by `restore/Restore-Dependencies.ps1`, which needs an elevated PowerShell.

```powershell
# ETS product database and knxprod.h
& (Join-Path $env:USERPROFILE "bin/OpenKNXproducer.exe") create -h include/knxprod.h src/Fan.xml

# Firmware
platformio run -e develop_RP2040
```

## Contributing

Contributions are welcome.

## License

GNU General Public License v3.0, see the LICENSE file.
