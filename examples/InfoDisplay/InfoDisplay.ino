#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include "epd_driver.h"
#include "pins.h"
#include "config.h"
#include "display_state.h"
#include "font/firasans.h"

uint8_t *framebuffer = NULL;
static const unsigned long REFRESH_MS = (unsigned long)REFRESH_INTERVAL_MIN * 60 * 1000;
unsigned long lastRefreshMs = 0;
bool firstPaintDone = false;

/* Set to 1 to print visible networks at startup (helps debug NO_AP_FOUND). */
#define WIFI_DEBUG_SCAN 1

static void wifi_scan_and_print(void) {
#if WIFI_DEBUG_SCAN
    Serial.println("\n[WiFi] Scanning for networks (ESP32 is 2.4 GHz only)...");
    int n = WiFi.scanNetworks();
    Serial.printf("[WiFi] Looking for SSID: \"%s\"\n", WIFI_SSID);
    if (n == 0) {
        Serial.println("[WiFi] No networks found (out of range or radio issue).");
    } else {
        Serial.printf("[WiFi] Found %d network(s):\n", n);
        for (int i = 0; i < n; i++) {
            Serial.printf("  %2d  %-32s  RSSI %4d  Ch %d\n",
                         i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
        }
    }
    WiFi.scanDelete();
#endif
}

static bool wifi_connect_with_retry(int max_attempts) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);   /* Keep radio awake during handshake; can fix 4WAY_HANDSHAKE_TIMEOUT */
    wifi_scan_and_print();
    WiFi.disconnect(true);
    delay(200);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    for (int i = 0; i < max_attempts; i++) {
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connect failed (4WAY_HANDSHAKE_TIMEOUT: check password is exact, try WPA2-only on router)");
    return false;
}

static void draw_loading_screen(void) {
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    int32_t cx = EPD_WIDTH / 2 - 80;
    int32_t cy = EPD_HEIGHT / 2 - 20;
    writeln((GFXfont *)&FiraSans, "Loading...", &cx, &cy, framebuffer);
    epd_poweron();
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    epd_poweroff();
}

static bool sync_ntp(void) {
    configTime((long)GMT_OFFSET_SEC, 0, "pool.ntp.org");
    struct tm t;
    for (int i = 0; i < 20; i++) {
        delay(500);
        if (getLocalTime(&t)) return true;
    }
    return false;
}

static void update_time_strings(void) {
    struct tm t;
    if (!getLocalTime(&t)) {
        snprintf(g_last_time.date_str, sizeof(g_last_time.date_str), "--");
        snprintf(g_last_time.time_str, sizeof(g_last_time.time_str), "--");
        g_last_time.time_section = SECTION_UNAVAILABLE;
        return;
    }
    strftime(g_last_time.date_str, sizeof(g_last_time.date_str), "%Y-%m-%d", &t);
    strftime(g_last_time.time_str, sizeof(g_last_time.time_str), "%H:%M", &t);
    g_last_time.time_section = SECTION_OK;
}

static void draw_date_time_on_framebuffer(void) {
    int32_t cx = 40;
    int32_t cy = 60;
    writeln((GFXfont *)&FiraSans, g_last_time.date_str, &cx, &cy, framebuffer);
    cy += FiraSans.advance_y;
    if (g_last_time.time_section == SECTION_OK)
        writeln((GFXfont *)&FiraSans, g_last_time.time_str, &cx, &cy, framebuffer);
    else
        writeln((GFXfont *)&FiraSans, "Offline", &cx, &cy, framebuffer);
}

static void draw_weather_on_framebuffer(void) {
    int32_t cx = 40;
    int32_t cy = 180;
    if (g_last_weather.weather_section == SECTION_OK || g_last_weather.weather_section == SECTION_STALE) {
        writeln((GFXfont *)&FiraSans, g_last_weather.condition, &cx, &cy, framebuffer);
        cy += FiraSans.advance_y;
        char buf[24];
        snprintf(buf, sizeof(buf), "%.1f %s", (double)g_last_weather.temperature, TEMP_UNIT_C ? "C" : "F");
        writeln((GFXfont *)&FiraSans, buf, &cx, &cy, framebuffer);
    } else {
        writeln((GFXfont *)&FiraSans, "Weather: --", &cx, &cy, framebuffer);
    }
}

static void draw_bitcoin_on_framebuffer(void) {
    int32_t cx = 40;
    int32_t cy = 300;
    if (g_last_price.price_section == SECTION_OK || g_last_price.price_section == SECTION_STALE) {
        char buf[32];
        snprintf(buf, sizeof(buf), "BTC $%.0f", (double)g_last_price.value_usd);
        writeln((GFXfont *)&FiraSans, buf, &cx, &cy, framebuffer);
    } else {
        writeln((GFXfont *)&FiraSans, "BTC: --", &cx, &cy, framebuffer);
    }
}

static void draw_logseq_todo_on_framebuffer(void) {
    int32_t cx = 40;
    int32_t cy = 420;
    if (g_last_logseq.section == SECTION_UNAVAILABLE) {
        writeln((GFXfont *)&FiraSans, "To-do: --", &cx, &cy, framebuffer);
        return;
    }
    if (g_last_logseq.section == SECTION_OK || g_last_logseq.section == SECTION_STALE) {
        if (g_last_logseq.count == 0) {
            writeln((GFXfont *)&FiraSans, "To-do: (none)", &cx, &cy, framebuffer);
            return;
        }
        for (int i = 0; i < g_last_logseq.count && i < LOGSEQ_MAX_ITEMS; i++) {
            char line[LOGSEQ_TITLE_LEN + 8];
            snprintf(line, sizeof(line), "%s %.48s", g_last_logseq.items[i].done ? "[x]" : "[ ]", g_last_logseq.items[i].title);
            writeln((GFXfont *)&FiraSans, line, &cx, &cy, framebuffer);
        }
    } else {
        writeln((GFXfont *)&FiraSans, "To-do: Unavailable", &cx, &cy, framebuffer);
    }
}

/* When new data is fetched: clear background (framebuffer + panel), then draw new data. */
static void refresh_display(void) {
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    draw_date_time_on_framebuffer();
    draw_weather_on_framebuffer();
    draw_bitcoin_on_framebuffer();
    draw_logseq_todo_on_framebuffer();
    epd_poweron();
    epd_clear();   /* clear panel to avoid ghosting before drawing new content */
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    epd_poweroff();
}

extern void weather_fetch(void);
extern void bitcoin_fetch(void);
extern void logseq_fetch(void);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("InfoDisplay starting");

    framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!framebuffer) {
        Serial.println("alloc memory failed !!!");
        while (1);
    }
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    display_state_init();

    epd_init();
    draw_loading_screen();
    firstPaintDone = true;

    if (!wifi_connect_with_retry(30)) {
        g_display_state = DISPLAY_OFFLINE;
        update_time_strings();
        refresh_display();
        lastRefreshMs = millis();
        return;
    }

    if (sync_ntp())
        update_time_strings();
    weather_fetch();
    bitcoin_fetch();
    logseq_fetch();

    if (g_last_time.time_section == SECTION_OK || g_last_weather.weather_section == SECTION_OK || g_last_price.price_section == SECTION_OK || g_last_logseq.section == SECTION_OK)
        g_display_state = (g_last_time.time_section == SECTION_OK && g_last_weather.weather_section == SECTION_OK && g_last_price.price_section == SECTION_OK && (g_last_logseq.section == SECTION_UNAVAILABLE || g_last_logseq.section == SECTION_OK)) ? DISPLAY_FULL : DISPLAY_PARTIAL;
    else
        g_display_state = DISPLAY_OFFLINE;

    refresh_display();
    lastRefreshMs = millis();
}

void loop() {
    unsigned long now = millis();
    if (now - lastRefreshMs >= REFRESH_MS) {
        lastRefreshMs = now;
        if (WiFi.status() == WL_CONNECTED) {
            if (sync_ntp()) update_time_strings();
            weather_fetch();
            bitcoin_fetch();
            logseq_fetch();
            if (g_display_state == DISPLAY_LOADING)
                g_display_state = DISPLAY_PARTIAL;
            if (g_last_time.time_section == SECTION_OK && g_last_weather.weather_section == SECTION_OK && g_last_price.price_section == SECTION_OK
                && (g_last_logseq.section == SECTION_UNAVAILABLE || g_last_logseq.section == SECTION_OK))
                g_display_state = DISPLAY_FULL;
        }
        refresh_display();
    }
    delay(1000);
}
