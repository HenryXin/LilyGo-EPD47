# Quickstart: InfoDisplay (Weather, Date/Time, Bitcoin)

**Branch**: `001-weather-datetime-btc`  
**Target**: LilyGo T4-4.7 (ESP32, 4.7" e-paper)

## Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- USB cable for the LilyGo T4-4.7
- WiFi network (2.4 GHz) for the device

## 1. Open the project

From the repository root:

```bash
cd examples/InfoDisplay
```

Or in VS Code: open the `LilyGo-EPD47` repo and set the project folder to `examples/InfoDisplay`.

## 2. Configure (optional)

Edit configuration so the device has:

- **Weather location**: Set latitude/longitude (or leave defaults).
- **Temperature unit**: Celsius or Fahrenheit.
- **WiFi**: SSID and password (e.g. in `config.h` or via the method the firmware expects).
- **Timezone**: GMT offset in seconds for local time.
- **Logseq** (optional): Server base URL and API token (compile-time only) to show today's to-do list.

If the project uses a `config.h` or `config.example.h`, copy the example and fill in:

- `WIFI_SSID`, `WIFI_PASS`
- `WEATHER_LAT`, `WEATHER_LON`
- `TEMP_UNIT_C` (1 = °C, 0 = °F)
- `GMT_OFFSET_SEC`
- `LOGSEQ_SERVER_URL` (e.g. `http://192.168.1.10:12315`) and `LOGSEQ_API_TOKEN` (from Logseq: Developer mode → Manage tokens) if using the to-do feature.

Exact names and mechanism follow the [config contract](./contracts/config-schema.md).

## 3. Build

Using PlatformIO:

```bash
pio run -e t4-4-7
```

Or in VS Code: **PlatformIO: Build** and select the `t4-4-7` environment.

Board is `lilygo-t4-47`; libraries and EPD driver are taken from the repo (`lib_dir = ../..`, `boards_dir = ../../platformio/boards`).

## 4. Upload

Connect the LilyGo via USB, then:

```bash
pio run -e t4-4-7 -t upload
```

Or in VS Code: **PlatformIO: Upload**. If the port is wrong, set `upload_port` in `platformio.ini` (e.g. `COM3` or `/dev/ttyUSB0`).

## 5. Monitor (optional)

Serial monitor at 115200 baud:

```bash
pio device monitor -b 115200
```

Use this to confirm WiFi connection, NTP sync, and API fetch logs.

## 6. Expected behavior

1. **First boot**: Display shows “Loading...” until the first successful data fetch.
2. **With WiFi**: Device fetches time (NTP), weather (Open-Meteo), Bitcoin price (CoinGecko), and (if configured) Logseq today's to-do list; updates every 15 minutes.
3. **Without WiFi**: Shows last data or “Offline” / “No data” as per spec.

## Troubleshooting

- **No WiFi**: Check SSID/password and 2.4 GHz; ensure serial log for connection errors.
- **No time**: NTP needs internet; confirm `configTime()` is called after WiFi connect and that `getLocalTime()` eventually succeeds.
- **No weather/price**: Check HTTP client and JSON parsing; confirm APIs are reachable (no firewall blocking).
- **No Logseq to-dos**: Ensure Logseq has Developer mode and API enabled, token created; `LOGSEQ_SERVER_URL` and `LOGSEQ_API_TOKEN` set in config; device can reach the Logseq host (e.g. same LAN).
- **Build errors**: Ensure you are in `examples/InfoDisplay` and that the repo root contains the EPD driver and `platformio/boards` (with `lilygo-t4-47`).

## References

- [Spec](./spec.md) – requirements and clarifications  
- [Plan](./plan.md) – technical context and structure  
- [Research](./research.md) – API choices (Open-Meteo, CoinGecko, NTP)  
- [Data model](./data-model.md) – entities and display state  
- [Config contract](./contracts/config-schema.md) – configuration schema
