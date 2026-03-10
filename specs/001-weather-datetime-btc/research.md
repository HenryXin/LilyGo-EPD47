# Research: Weather, Date/Time & Bitcoin Info Display

**Branch**: `001-weather-datetime-btc` | **Phase 0 output**

## 1. Weather data source

**Decision**: Use **Open-Meteo** API for current weather (condition + temperature) by location.

**Rationale**:
- No API key required; single HTTP GET with latitude/longitude (or location name) suffices.
- JSON response; well-suited to Arduino/ESP32 with ArduinoJson.
- Free for non-commercial use; adequate rate limits for 15-minute refresh.
- Returns `current_conditions` (temperature, weather code); optional `feels_like`; supports Celsius and Fahrenheit via parameter.

**Alternatives considered**:
- OpenWeatherMap: requires API key and sign-up; free tier sufficient but adds configuration and key handling.
- Other key-less services: less documented or less stable; Open-Meteo is widely used and documented.

**Endpoint pattern**: `https://api.open-meteo.com/v1/forecast?latitude=LAT&longitude=LON&current=temperature_2m,relative_humidity_2m,weather_code&temperature_unit=celsius|fahrenheit`

---

## 2. Bitcoin price data source

**Decision**: Use **CoinGecko** public API `/simple/price` for BTC in USD.

**Rationale**:
- No API key for the public endpoint; simple GET returns JSON.
- Direct support for `ids=bitcoin&vs_currencies=usd`; minimal parsing (single number).
- Free tier rate-limited (e.g. ~10–50 req/min); sufficient for 15-minute refresh.

**Alternatives considered**:
- Other crypto APIs (e.g. Binance, CoinCap): often require keys or more complex responses; CoinGecko is simple and key-free for this use case.

**Endpoint pattern**: `https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd`

---

## 3. Date/time source

**Decision**: Use **NTP** via ESP32 built-in support (`configTime` / `getLocalTime` from `time.h`).

**Rationale**:
- No extra hardware; WiFi connection is enough.
- Standard Arduino/ESP32 pattern: connect WiFi → `configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org")` → wait for sync → `getLocalTime()` or `localtime()`.
- Timezone can be fixed in config (e.g. UTC offset); no dependency on weather/price APIs for time.

**Alternatives considered**:
- RTC module: adds hardware and battery; not required if NTP is available after WiFi connect.
- Time from weather API: possible but ties time to weather availability; NTP is dedicated and reliable.

**Best practice**: Call `configTime()` only after `WiFi.status() == WL_CONNECTED`; retry with short delay until `getLocalTime()` succeeds (or timeout) before showing time.

---

## 4. JSON parsing on ESP32

**Decision**: Use **ArduinoJson** (e.g. v6) for parsing Open-Meteo and CoinGecko responses.

**Rationale**:
- Common choice for ESP32/Arduino; supports small payloads and static allocation to limit heap use.
- Documented patterns for HTTP + JSON; reduces risk of brittle manual parsing.
- Open-Meteo and CoinGecko responses are small; a single `JsonDocument` per request is sufficient.

**Alternatives considered**:
- Manual string parsing: possible for very small responses but error-prone and harder to maintain.
- Other JSON libraries: ArduinoJson is the de facto standard in this ecosystem.

---

## 5. Configuration (location, temp unit)

**Decision**: Use a **compile-time or single config header** (e.g. `config.h`) for weather location (lat/lon or city), timezone offset, and temperature unit (C/F). Optional: store in NVS after first run for future “configurable at setup” without reflash.

**Rationale**:
- Spec: “single fixed location set at setup (config or compile-time)” and “temperature user-configurable (C or F)” — a single place (header or NVS) keeps scope clear.
- Avoids WiFi/captive-portal or complex UI on device; aligns with “fixed” choices for this version.

**Alternatives considered**:
- Web config portal: more flexible but out of scope for “fixed at setup.”
- Only compile-time: simplest; NVS can be added later if “setup” is interpreted as one-time device config.

---

## 6. Display refresh when new data is fetched

**Decision**: When new data is fetched, **clear the background, then draw the new data**. Concretely: (1) clear the framebuffer to white (memset 0xFF), (2) power on the panel and run a full display clear cycle (`epd_clear()`) to avoid ghosting, (3) draw the new content into the framebuffer and push it to the panel, (4) power off.

**Rationale**:
- E-paper can retain previous content (ghosting); a full clear before drawing ensures a clean update.
- Explicit “clear then draw” matches user expectation and reduces artifacts when time/weather/price change.

**Alternatives considered**:
- Partial update (only changed regions): more complex and driver-dependent; full clear is simpler and reliable.
- Framebuffer-only clear: may leave ghosting on the physical panel; adding `epd_clear()` addresses that.

---

## 7. Logseq today's to-do list (Graph API)

**Decision**: Use the **Logseq HTTP API** (often referred to as Graph API in plugin docs): POST to `{base_url}/api` with JSON body `{"method":"logseq.Editor.getPageBlocksTree","args":["PAGE_NAME"]}` and `Authorization: Bearer {token}`. The "today" journal page name is derived from device date (e.g. `yyyy_MM_dd` default; configurable in Logseq via `:journal/page-title-format`). Filter returned blocks for TODO/DONE markers to get title and status.

**Rationale**:
- Spec mandates Logseq Graph API and compile-time server URL + API token; this is the official plugin/HTTP interface.
- Single endpoint pattern: one POST per refresh; response is a tree of blocks; each block can have `marker` (e.g. "TODO", "DONE") and `content` (title).
- Works with self-hosted or desktop Logseq with API enabled (Developer mode → enable API → create token). Base URL is typically `http://HOST:12315` (default port).
- ESP32: HTTPClient POST with Bearer header; parse JSON (ArduinoJson) for block list; extract `content` and `marker` per block; display title + done/pending.

**Alternatives considered**:
- Custom plugin exposing a dedicated "today to-dos" endpoint: would require maintaining a plugin and different contract; spec chose official API.
- File-based export (e.g. export today as JSON): not a standard Logseq feature and would require file sync; HTTP API is the supported remote access path.

**Endpoint pattern**: `POST {LOGSEQ_SERVER_URL}/api` with body `{"method":"logseq.Editor.getPageBlocksTree","args":["YYYY_MM_DD"]}` (today's journal page name from device date) and header `Authorization: Bearer {LOGSEQ_API_TOKEN}`. Response: JSON array of block objects; each block has `content`, `marker` (e.g. "TODO", "DONE"); flatten tree if needed to list all to-dos. Journal page title format: default `yyyy_MM_dd`; if Logseq instance uses another format, config may need to supply a format hint or the exact page name (out of scope for first version: assume default format).

---

## Summary table

| Concern            | Choice        | Rationale summary                    |
|--------------------|---------------|--------------------------------------|
| Weather API        | Open-Meteo    | No key, JSON, lat/lon, C/F support   |
| Bitcoin API        | CoinGecko     | No key, simple USD price endpoint    |
| Time               | NTP           | Built-in ESP32, WiFi-only             |
| Logseq to-do       | Logseq HTTP API | POST /api, getPageBlocksTree(today), Bearer token; filter TODO/DONE |
| JSON               | ArduinoJson   | Standard, small footprint             |
| Config             | config.h/NVS  | Fixed at setup; Logseq URL + token at compile-time |
| Display refresh    | Clear then draw | Full framebuffer + panel clear, then new content; reduces ghosting |

All NEEDS CLARIFICATION from Technical Context are resolved by the above and the existing spec.
