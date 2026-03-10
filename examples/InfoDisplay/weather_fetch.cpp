#include "display_state.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "config.h"

static const char OPEN_METEO_URL[] = "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f&current=temperature_2m,weather_code&temperature_unit=%s";

static const char* weather_code_to_condition(int code) {
    if (code == 0) return "Clear";
    if (code >= 1 && code <= 3) return "Cloudy";
    if (code >= 45 && code <= 48) return "Fog";
    if (code >= 51 && code <= 67) return "Rain";
    if (code >= 71 && code <= 77) return "Snow";
    if (code >= 80 && code <= 82) return "Showers";
    if (code >= 95 && code <= 99) return "Thunder";
    return "Unknown";
}

void weather_fetch(void) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (WEATHER_LAT < -90.f || WEATHER_LAT > 90.f || WEATHER_LON < -180.f || WEATHER_LON > 180.f) return;

    char url[256];
    snprintf(url, sizeof(url), OPEN_METEO_URL,
             (double)WEATHER_LAT, (double)WEATHER_LON,
             TEMP_UNIT_C ? "celsius" : "fahrenheit");

    HTTPClient http;
    http.begin(url);
    http.setTimeout(10000);
    int code = http.GET();

    if (code != HTTP_CODE_OK) {
        http.end();
        if (g_last_weather.weather_section != SECTION_UNAVAILABLE) g_last_weather.weather_section = SECTION_STALE;
        return;
    }

    String payload = http.getString();
    http.end();

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return;

    JsonObject cur = doc["current"];
    if (cur.isNull()) return;

    float temp = cur["temperature_2m"].as<float>();
    int wcode = cur["weather_code"].as<int>();

    snprintf(g_last_weather.condition, sizeof(g_last_weather.condition), "%s", weather_code_to_condition(wcode));
    g_last_weather.temperature = temp;
    g_last_weather.weather_section = SECTION_OK;
}
