# Implementation Plan: Weather, Date/Time & Bitcoin Info Display

**Branch**: `001-weather-datetime-btc` | **Date**: 2025-02-24 | **Spec**: [spec.md](./spec.md)  
**Input**: Feature specification from `/specs/001-weather-datetime-btc/spec.md`

## Summary

Display weather (condition + temperature in C/F), current date/time, Bitcoin price in USD, and Logseq today's to-do list on a LilyGo T4-4.7 e-paper device. Data is fetched from the internet; a single fixed location and 15-minute refresh interval are used for all sources. Show "Loading..." until first successful fetch; on loss of connectivity show last data or offline indicator. Temperature unit (C/F) is user-configurable (config/setup). Logseq server URL and API token are configurable at compile-time only; each to-do is shown with title and status (done vs pending).

**Display refresh rule**: When new data is fetched, clear the background (framebuffer and panel), then draw the new data. This avoids ghosting and ensures a clean update.

Technical approach: ESP32 firmware (Arduino, PlatformIO) in `examples/InfoDisplay`; WiFi + NTP for time; free HTTP APIs for weather and Bitcoin price; Logseq Graph API (HTTP POST with Bearer token) for today's to-dos; repo EPD driver for rendering; config (location, temp unit, Logseq URL, Logseq API key) at compile-time or NVS.

## Technical Context

**Language/Version**: C++ (Arduino framework, ESP32 core)  
**Primary Dependencies**: Arduino (WiFi, HTTPClient, time/NTP), EPD driver and pins from repo (`lib_dir = ../..`), JSON parsing (ArduinoJson or minimal manual parse)  
**Storage**: NVS for optional persistence of last-display state and config (location, temp unit); no database  
**Testing**: PlatformIO build; manual on-device verification; optional unit tests for parsing  
**Target Platform**: ESP32 (LilyGo T4-4.7), 4.7" e-paper display  
**Project Type**: Embedded firmware / single-purpose display app  
**Performance Goals**: First display within 1 minute of connectivity; refresh every 15 minutes  
**Constraints**: WiFi-only; low power consideration for e-paper; free, no-auth data sources for weather/BTC; Logseq requires API token (compile-time config).  
**Scale/Scope**: Single device, four data domains (time, weather, Bitcoin, Logseq today to-do), one fixed location, one Logseq server

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

Constitution file (`.specify/memory/constitution.md`) is a generic template with placeholders; no project-specific gates defined. No violations assumed. Proceed with Phase 0.

## Project Structure

### Documentation (this feature)

```text
specs/001-weather-datetime-btc/
├── plan.md              # This file
├── research.md          # Phase 0: API choices, patterns
├── data-model.md        # Phase 1: entities, state
├── quickstart.md        # Phase 1: build, flash, configure
├── contracts/           # Phase 1: config schema (if applicable)
└── tasks.md             # Phase 2: /speckit.tasks (not created by plan)
```

### Source Code (this example)

```text
examples/InfoDisplay/
├── platformio.ini       # board lilygo-t4-47, src_dir=., lib_dir=../..
├── InfoDisplay.ino      # main setup/loop
├── pins.h               # board pins (shared pattern with drawExample)
├── config.h             # (to add) location, temp unit, refresh interval
└── (optional) *.cpp/*.h # networking, fetch, render modules
```

**Structure Decision**: Single example app under `examples/InfoDisplay`; shared EPD and platform code from repo root (`../..`). No separate backend/frontend; all logic and display in one firmware image.

## Complexity Tracking

Not applicable; no constitution violations to justify.
