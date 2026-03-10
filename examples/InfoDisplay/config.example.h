/**
 * Example configuration for InfoDisplay.
 * Copy to config.h and set your WiFi and location.
 *
 * Contract: specs/001-weather-datetime-btc/contracts/config-schema.md
 */
#ifndef CONFIG_EXAMPLE_H
#define CONFIG_EXAMPLE_H

#define WIFI_SSID       "YourSSID"       /* Exact network name; ESP32 is 2.4 GHz only (NO_AP_FOUND = wrong SSID or 5 GHz only) */
#define WIFI_PASS       "YourPassword"   /* WiFi password (exact, case-sensitive; 4WAY_HANDSHAKE_TIMEOUT often means wrong password or try WPA2-only on router) */
#define WEATHER_LAT     52.52f          /* Latitude for Open-Meteo (-90..90) */
#define WEATHER_LON     13.41f          /* Longitude for Open-Meteo (-180..180) */
#define TEMP_UNIT_C     1               /* 1 = Celsius, 0 = Fahrenheit */
#define GMT_OFFSET_SEC  3600            /* Local offset from GMT in seconds (e.g. 3600 = UTC+1) */
#define REFRESH_INTERVAL_MIN 15u        /* Minutes between refresh (fixed 15 for this version) */

/* Logseq today to-do (compile-time only). Leave empty to disable to-do section. */
#define LOGSEQ_SERVER_URL ""            /* e.g. "http://127.0.0.1:12315" (same machine) or "http://192.168.1.x:12315" (LAN IP for ESP32); no trailing slash */
#define LOGSEQ_API_TOKEN ""             /* Bearer token from Logseq: Developer mode → Manage tokens */
/* Optional: bridge URL when Logseq API returns empty (known bug). Run scripts/logseq_todos_bridge.py on your PC and set e.g. "http://192.168.0.28:8765/todos" */
#define LOGSEQ_TODOS_URL ""

#endif
