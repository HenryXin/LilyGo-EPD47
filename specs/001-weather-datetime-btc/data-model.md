# Data Model: Weather, Date/Time & Bitcoin Info Display

**Branch**: `001-weather-datetime-btc` | **Phase 1 output**

Entities are derived from the feature spec Key Entities and requirements. No persistent database; in-memory (and optional NVS) state for display and resilience.

---

## 1. Weather

Represents current conditions for a single fixed location (set at setup).

| Field / concept      | Type / rule | Notes |
|----------------------|-------------|--------|
| location             | Fixed (config) | Lat/lon or city identifier for API (e.g. Open-Meteo) |
| condition             | String / enum | Human-readable or weather code (e.g. sunny, cloudy, rain) |
| temperature           | Numeric      | In user-selected unit (C or F) |
| unit                  | Enum         | `celsius` \| `fahrenheit` (user-configurable) |
| last_updated           | Timestamp    | When this snapshot was fetched (optional, for staleness) |

**Validation**: Temperature in reasonable range (e.g. -50 to 60 °C or equivalent in F). Condition non-empty when available.

**State**: No lifecycle beyond “current snapshot”; replaced on each successful fetch. On fetch failure, previous snapshot retained for display (FR-006, FR-008).

---

## 2. Date/Time

Represents the current moment in a user-relevant timezone.

| Field / concept | Type / rule | Notes |
|------------------|-------------|--------|
| date             | Date        | Calendar date (e.g. day/month/year) |
| time             | Time        | Time of day (e.g. HH:MM or HH:MM:SS) |
| timezone_offset  | Fixed (config) | GMT offset in seconds (or timezone string) for NTP/local display |

**Validation**: Valid after NTP sync; display “--” or loading state until synced.

**State**: Updated from RTC/NTP at each refresh (or more often for time-only if we advance locally between refreshes). On NTP failure, last known time can be shown with optional “stale” indicator.

---

## 3. BitcoinPrice

Represents the current BTC price in USD.

| Field / concept | Type / rule | Notes |
|------------------|-------------|--------|
| value_usd        | Numeric     | Price in USD (e.g. from CoinGecko) |
| currency         | Fixed       | `USD` only (spec) |
| last_updated     | Timestamp   | When this value was fetched (optional) |

**Validation**: Value ≥ 0; display “--” or “Unavailable” when fetch fails and no previous value.

**State**: Replaced on each successful fetch; on failure, last value retained (FR-006, FR-008).

---

## 4. LogseqTodayTodo (today's to-do item)

Represents a single task on the Logseq "today" journal page (FR-010, FR-012).

| Field / concept | Type / rule | Notes |
|------------------|-------------|--------|
| title            | String      | Block content (task title) |
| status           | Enum        | `pending` (marker TODO) \| `done` (marker DONE) |
| (order)          | Optional    | Preserve list order from API for display |

**Validation**: Title may be empty; display as-is or placeholder. Status derived from block `marker` (TODO → pending, DONE → done; other markers implementation-defined).

**State**: List of items replaced on each successful Logseq fetch; on failure, last list retained (FR-006, edge case: server unreachable / 401 / empty today). Empty list when today's page has no to-do blocks is valid.

---

## 5. DisplayState

What is shown on the e-paper at any moment. Supports loading, full data, and partial/offline states.

| State / concept   | When used | Display behavior |
|--------------------|-----------|-------------------|
| Loading            | Before first successful fetch (any data) | Show “Loading...” or similar (FR-009) |
| Full               | All four data domains fetched at least once | Show date/time, weather, Bitcoin price, Logseq today to-do |
| Partial            | One or more domains failed last fetch | Show last good data for successful domains; “Unavailable” or offline for failed |
| Offline / No data  | No internet on first run or all fetches failed | Offline indicator and/or last data if any |

**Per-section state** (optional refinement): Each of time, weather, price, and Logseq to-do can have `ok` | `stale` | `unavailable` so we can show last value with indicator vs “Unavailable” when we never had data.

---

## 6. Config (in-memory / NVS or config.h)

Runtime or build-time configuration; not a “domain entity” but part of the model.

| Field           | Type / rule | Notes |
|-----------------|-------------|--------|
| weather_location_lat | Float (config) | Latitude for Open-Meteo |
| weather_location_lon | Float (config) | Longitude for Open-Meteo |
| temp_unit       | Enum (config) | `celsius` \| `fahrenheit` |
| timezone_gmt_offset_sec | Integer (config) | For NTP/local time |
| refresh_interval_min   | Fixed       | 15 (spec) |
| logseq_server_url     | String (compile-time) | Base URL for Logseq API (e.g. http://host:12315) |
| logseq_api_token      | String (compile-time) | Bearer token for Logseq API (required) |

**Persistence**: Can live in `config.h` only, or in NVS for “setup once” without reflash. Logseq URL and token are compile-time only per spec. No user-facing schema versioning for this version.

---

## Relationships

- **DisplayState** is composed of: optional **Date/Time**, optional **Weather**, optional **BitcoinPrice**, optional list of **LogseqTodayTodo**, plus loading/error flags per section.
- **Config** is read at startup and (if stored) when loading from NVS; it determines **Weather** location and unit and **Date/Time** timezone, and **Logseq** server URL and API token.
- No relationships between Weather, Date/Time, BitcoinPrice, and Logseq to-do list other than being shown together on the same display.

---

## State transitions (high level)

1. **Boot** → Connect WiFi → **Loading** (show “Loading...”).
2. **Loading** → When first successful fetch completes for at least one domain → **Partial** or **Full** (show data).
3. **Full / Partial** → Every 15 min: attempt fetch for all domains; on success update in-memory entities; on failure keep last value and mark section unavailable or retry next cycle. On each paint: clear framebuffer and panel, then draw new content (per plan display refresh rule).
4. **Full / Partial** → WiFi lost or all fetches fail → **Partial** or **Offline** (show last data and/or offline indicator).
5. Logseq fetch: same 15 min cycle; on success update in-memory list of **LogseqTodayTodo**; on failure (unreachable, 401, parse error) keep last list and mark to-do section unavailable until next success.

No formal state machine required; the above suffices for implementation and acceptance tests.
