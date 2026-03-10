# Contract: Application configuration

**Branch**: `001-weather-datetime-btc`  
**Consumer**: InfoDisplay firmware (this example)  
**Format**: Build-time (`config.h`) and/or NVS key-value

The firmware expects the following configuration. Missing or invalid values fall back to documented defaults or safe behavior (e.g. skip weather if location missing).

## Required / expected entries

| Symbol / NVS key   | Type    | Example | Description |
|--------------------|---------|---------|-------------|
| `WEATHER_LAT`      | float   | 52.52   | Latitude for weather API |
| `WEATHER_LON`      | float   | 13.41   | Longitude for weather API |
| `TEMP_UNIT_C`      | bool    | 1       | 1 = Celsius, 0 = Fahrenheit |
| `GMT_OFFSET_SEC`   | int32_t | 3600    | Local time offset from GMT in seconds (e.g. 3600 for UTC+1) |
| `WIFI_SSID`        | string  | "MySSID"| WiFi SSID (if not compile-time only) |
| `WIFI_PASS`        | string  | "***"   | WiFi password (if not compile-time only) |

## Logseq (compile-time only; required for to-do feature)

| Symbol / NVS key   | Type   | Example | Description |
|--------------------|--------|---------|-------------|
| `LOGSEQ_SERVER_URL`  | string | "http://192.168.1.10:12315" | Base URL of Logseq server (no trailing slash); default API port 12315 |
| `LOGSEQ_API_TOKEN`   | string | "your-token" | Bearer token for Logseq HTTP API (create in Logseq: Developer mode → Manage tokens) |

If either is missing or empty, the to-do section shows "Unavailable" or is skipped (implementation-defined). No NVS keys for Logseq in this version; compile-time only per spec.

## Optional

| Symbol / NVS key   | Type  | Default | Description |
|--------------------|-------|---------|-------------|
| `REFRESH_INTERVAL_MIN` | uint16_t | 15 | Minutes between full refresh (spec: fixed 15) |

## Contract rules

1. **Location**: If both `WEATHER_LAT` and `WEATHER_LON` are present and in valid ranges (-90..90, -180..180), weather is fetched; otherwise weather section shows "Unavailable" or is skipped.
2. **Temp unit**: If `TEMP_UNIT_C` is missing, implementation may default to Celsius.
3. **Time**: If `GMT_OFFSET_SEC` is missing, 0 (UTC) can be used; display still shows time after NTP sync.
4. **WiFi**: If SSID/password are not configured (e.g. empty), device may stay in loading/offline until provisioned (implementation-defined).
5. **Logseq**: If `LOGSEQ_SERVER_URL` or `LOGSEQ_API_TOKEN` are missing/empty, the to-do list is not fetched; to-do section shows "Unavailable" or is omitted.

## Validation

- Latitude in [-90, 90], longitude in [-180, 180].
- `REFRESH_INTERVAL_MIN` > 0 and reasonable (e.g. ≤ 60) if present.

This contract is satisfied by providing a `config.h` (or equivalent) and/or NVS keys with the above names and types; no external system depends on it.
