# Tasks: Weather, Date/Time & Bitcoin Info Display

**Input**: Design documents from `/specs/001-weather-datetime-btc/`  
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**Tests**: Not requested in spec; no test tasks included. Manual on-device verification per quickstart.md.

**Organization**: Tasks grouped by user story so each story can be implemented and validated independently.

## Format: `[ID] [P?] [Story?] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: User story (US1, US2, US3, US4, US5)
- Paths are relative to repository root; example code lives in `examples/InfoDisplay/`.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and config contract implementation.

- [x] T001 Add ArduinoJson to lib_deps in examples/InfoDisplay/platformio.ini
- [x] T002 [P] Create config.h with WIFI_SSID, WIFI_PASS, WEATHER_LAT, WEATHER_LON, TEMP_UNIT_C, GMT_OFFSET_SEC per specs/001-weather-datetime-btc/contracts/config-schema.md in examples/InfoDisplay/config.h
- [x] T003 [P] Create config.example.h with placeholder values and comments in examples/InfoDisplay/config.example.h
- [x] T028 [P] Add LOGSEQ_SERVER_URL and LOGSEQ_API_TOKEN (compile-time) to config.h and config.example.h with comments per specs/001-weather-datetime-btc/contracts/config-schema.md in examples/InfoDisplay/

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: WiFi, display init, and loading state. Must be complete before any user story.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [x] T004 Implement WiFi connect with retry using config.h credentials in examples/InfoDisplay/InfoDisplay.ino (or dedicated module)
- [x] T005 Initialize EPD (epd_init, framebuffer alloc) and show "Loading..." on first paint in examples/InfoDisplay/InfoDisplay.ino
- [x] T006 Implement display state enum (Loading, Full, Partial, Offline) and last-value storage for time, weather, price in examples/InfoDisplay/InfoDisplay.ino or examples/InfoDisplay/display_state.h
- [x] T029 Add last-value storage for Logseq to-do list (optional list of title+status) in examples/InfoDisplay/InfoDisplay.ino or examples/InfoDisplay/display_state.h

**Checkpoint**: WiFi connects, EPD shows Loading; ready for US1/US2/US3/US5.

---

## Phase 3: User Story 1 – See current date and time (Priority: P1) – MVP

**Goal**: User sees current date and time on the display (clock); works without weather or Bitcoin.

**Independent Test**: Power device, connect WiFi; display shows current date and time within one minute; without internet, last time or offline indicator.

- [x] T007 [US1] Sync time via NTP (configTime, getLocalTime) using GMT_OFFSET_SEC from config in examples/InfoDisplay/InfoDisplay.ino or examples/InfoDisplay/ntp_time.cpp
- [x] T008 [US1] Format and store current date and time (date string, time string) for display in examples/InfoDisplay/InfoDisplay.ino or examples/InfoDisplay/ntp_time.cpp
- [x] T009 [US1] Draw date and time on EPD in a readable layout (reserved region) in examples/InfoDisplay/InfoDisplay.ino or examples/InfoDisplay/display_render.cpp
- [x] T010 [US1] On NTP failure or no WiFi, show last known date/time or "Offline" for time section in examples/InfoDisplay/InfoDisplay.ino

**Checkpoint**: User Story 1 complete; display works as a clock.

---

## Phase 4: User Story 2 – See current weather (Priority: P2)

**Goal**: User sees weather condition and temperature (C or F) for the fixed location.

**Independent Test**: With WiFi and valid config, display shows condition and temperature matching a trusted source; without internet, last weather or offline.

- [x] T011 [P] [US2] Implement HTTP GET to Open-Meteo (latitude, longitude, temperature_unit from config) and parse JSON with ArduinoJson in examples/InfoDisplay/weather_fetch.cpp or examples/InfoDisplay/weather_fetch.h
- [x] T012 [US2] Map Open-Meteo weather_code to short condition string (e.g. sunny, cloudy, rain) in examples/InfoDisplay/weather_fetch.cpp
- [x] T013 [US2] Store weather condition and temperature (in configured unit) for display; handle parse/HTTP errors without crashing in examples/InfoDisplay/weather_fetch.cpp
- [x] T014 [US2] Draw weather section (condition + temperature) on EPD in examples/InfoDisplay/display_render.cpp or InfoDisplay.ino
- [x] T015 [US2] On fetch failure, show last weather or "Unavailable" for weather section in examples/InfoDisplay/InfoDisplay.ino

**Checkpoint**: User Stories 1 and 2 work independently.

---

## Phase 5: User Story 3 – See current Bitcoin price (Priority: P3)

**Goal**: User sees current BTC price in USD.

**Independent Test**: With WiFi, display shows BTC price in USD; without internet, last price or offline.

- [x] T016 [P] [US3] Implement HTTP GET to CoinGecko simple/price (ids=bitcoin, vs_currencies=usd) and parse JSON in examples/InfoDisplay/bitcoin_fetch.cpp or examples/InfoDisplay/bitcoin_fetch.h
- [x] T017 [US3] Store BTC price (USD) for display; handle parse/HTTP errors in examples/InfoDisplay/bitcoin_fetch.cpp
- [x] T018 [US3] Draw Bitcoin price (USD) on EPD in examples/InfoDisplay/display_render.cpp or InfoDisplay.ino
- [x] T019 [US3] On fetch failure, show last price or "Unavailable" for price section in examples/InfoDisplay/InfoDisplay.ino

**Checkpoint**: User Stories 1, 2, and 3 all work independently.

---

## Phase 6: User Story 4 – Information stays current (Priority: P4)

**Goal**: Data refreshes every 15 minutes; failures keep last data and retry next cycle.

**Note**: Logseq fetch is implemented in Phase 7 (T030); T020/T021 assume that entry point when configured.

**Independent Test**: Leave device running; after 15 minutes display updates; on network error, last data shown and retry on next interval.

- [x] T020 [US4] Add 15-minute refresh timer (fixed interval) and trigger fetch for time, weather, Bitcoin, and Logseq (when configured) in examples/InfoDisplay/InfoDisplay.ino
- [x] T021 [US4] On refresh: update NTP/time, call weather fetch, Bitcoin fetch, and Logseq fetch (if LOGSEQ_SERVER_URL and LOGSEQ_API_TOKEN set); update display with new or last values per section in examples/InfoDisplay/InfoDisplay.ino
- [x] T022 [US4] On partial failure (one API fails), keep last value for that section and still update others; show "Unavailable" only when no previous value in examples/InfoDisplay/InfoDisplay.ino
- [x] T023 [US4] Transition from Loading to Full/Partial when at least one domain succeeds; never leave Loading stuck if WiFi is connected in examples/InfoDisplay/InfoDisplay.ino

**Checkpoint**: US1–US4 complete; display refreshes and handles offline. US5 adds Logseq to-do.

---

## Phase 7: User Story 5 – See Logseq today to-do list (Priority: P5)

**Goal**: User sees today's to-do list from Logseq (title + done/pending per item); server URL and API token configured at compile-time.

**Independent Test**: With WiFi, valid LOGSEQ_SERVER_URL and LOGSEQ_API_TOKEN, and Logseq server reachable with today's journal containing TODO/DONE blocks, display shows to-do items with title and status; without config or on 401/unreachable, show last list or "Unavailable" for to-do section.

- [x] T030 [P] [US5] Implement Logseq HTTP client: POST to {LOGSEQ_SERVER_URL}/api with body {"method":"logseq.Editor.getPageBlocksTree","args":["YYYY_MM_DD"]} and Authorization Bearer LOGSEQ_API_TOKEN; derive today page name from device date (yyyy_MM_dd) in examples/InfoDisplay/logseq_fetch.cpp or examples/InfoDisplay/logseq_fetch.h
- [x] T031 [US5] Parse Logseq API response (ArduinoJson): extract blocks with marker TODO or DONE and content (title); build in-memory list of LogseqTodayTodo (title, status pending/done); handle empty response or parse errors in examples/InfoDisplay/logseq_fetch.cpp
- [x] T032 [US5] Store parsed to-do list for display; on HTTP error (401, timeout, unreachable) or parse error retain last list and mark to-do section unavailable in examples/InfoDisplay/logseq_fetch.cpp and InfoDisplay.ino
- [x] T033 [US5] Draw to-do section on EPD: each item with title and status (e.g. checkbox or "done"/"pending" label) in examples/InfoDisplay/display_render.cpp or InfoDisplay.ino
- [x] T034 [US5] When LOGSEQ_SERVER_URL or LOGSEQ_API_TOKEN is missing or empty, skip Logseq fetch and show "Unavailable" or omit to-do section per contracts/config-schema.md in examples/InfoDisplay/InfoDisplay.ino

**Checkpoint**: User Story 5 complete; display shows Logseq today to-do when configured; refresh includes Logseq (same 15 min).

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Layout, readability, display refresh rule, and validation.

- [x] T024 [P] Ensure layout distinguishes date/time, weather, and Bitcoin (FR-007) in examples/InfoDisplay/display_render.cpp or InfoDisplay.ino
- [x] T035 [P] Ensure layout includes to-do section and distinguishes date/time, weather, Bitcoin, and Logseq to-do (FR-007, FR-012) in examples/InfoDisplay/display_render.cpp or InfoDisplay.ino
- [x] T025 Validate config (lat/lon range, TEMP_UNIT_C) at startup and fallback to safe defaults in examples/InfoDisplay/InfoDisplay.ino or examples/InfoDisplay/config.h
- [x] T026 [P] Ensure refresh clears background (framebuffer + epd_clear) then draws new data per plan display refresh rule in examples/InfoDisplay/InfoDisplay.ino
- [x] T027 Run quickstart.md build/upload/monitor steps and confirm behavior per spec acceptance scenarios
- [ ] T036 Run quickstart.md and verify Logseq to-do section (when configured), offline/401 behavior, and 15-min refresh including Logseq per spec edge cases

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies.
- **Phase 2 (Foundational)**: Depends on Phase 1 (config, lib_deps). Blocks all user stories.
- **Phase 3 (US1)**: Depends on Phase 2. No dependency on US2/US3/US5.
- **Phase 4 (US2)**: Depends on Phase 2; can use same display/framebuffer as US1. Independently testable.
- **Phase 5 (US3)**: Depends on Phase 2; same display. Independently testable.
- **Phase 6 (US4)**: Depends on Phases 3–5 (time, weather, Bitcoin); refresh loop includes Logseq when implemented.
- **Phase 7 (US5)**: Depends on Phase 2 (display, config); can be implemented after or in parallel with US2/US3. Refresh (US4) includes Logseq once US5 is done.
- **Phase 8 (Polish)**: Depends on Phase 6 and Phase 7.

### User Story Dependencies

- **US1 (P1)**: After Foundational only. MVP.
- **US2 (P2)**: After Foundational; shares display and config with US1.
- **US3 (P3)**: After Foundational; shares display.
- **US4 (P4)**: After US1, US2, US3 (needs time, weather, Bitcoin; Logseq optional when US5 done).
- **US5 (P5)**: After Foundational; shares display and config. Independently testable; refresh (US4) includes Logseq when configured.

### Parallel Opportunities

- T002, T003, and T028 can run in parallel (Phase 1).
- T011 (weather fetch) and T016 (Bitcoin fetch) are in different files and can be implemented in parallel after Foundational.
- T030 (Logseq fetch module) can be implemented in parallel with other fetch modules after Foundational.
- T024, T035 (layout), and T026 (clear-then-draw) can be done in parallel with T025 (config validation) in Phase 8.

---

## Parallel Example: User Story 2 and 3

```text
# After Phase 2, weather and Bitcoin fetch modules are independent:
Task T011: Implement Open-Meteo fetch in examples/InfoDisplay/weather_fetch.cpp
Task T016: Implement CoinGecko fetch in examples/InfoDisplay/bitcoin_fetch.cpp
# Then T012–T015 (weather display) and T017–T019 (Bitcoin display) in order within each story.
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (config, ArduinoJson).
2. Complete Phase 2: Foundational (WiFi, EPD, Loading).
3. Complete Phase 3: User Story 1 (NTP, date/time on display).
4. **STOP and VALIDATE**: Display shows current date/time; works as clock.
5. Demo/deploy if desired.

### Incremental Delivery

1. Setup + Foundational → device shows "Loading..." and connects WiFi.
2. Add US1 → display shows date/time (MVP).
3. Add US2 → display shows weather.
4. Add US3 → display shows Bitcoin price.
5. Add US4 → 15-min refresh and offline handling.
6. Add US5 → Logseq today to-do (config + fetch + display); refresh includes Logseq when configured.
7. Polish → layout (including to-do section) and validation; run full quickstart check.

### Single-Developer Order

Execute T001–T006, T028–T029 (Setup + Logseq config and state), then T007–T010 (US1), T011–T015 (US2), T016–T019 (US3), T020–T023 (US4), T030–T034 (US5), then T024–T027, T035–T036 (Polish).

---

## Notes

- [P] tasks use different files or have no ordering requirement.
- [USn] maps each task to a user story for traceability.
- No automated test tasks; verify manually per quickstart.md and spec acceptance scenarios.
- Commit after each task or logical group.
- Paths assume repo root is LilyGo-EPD47; examples/InfoDisplay/ is the app directory.
- US5 (Logseq) tasks T028–T029, T030–T034, T035–T036 are new; all use checklist format with IDs and file paths.
