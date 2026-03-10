#include "display_state.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

static const char COINGECKO_URL[] = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd";

void bitcoin_fetch(void) {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.begin(COINGECKO_URL);
    http.setTimeout(10000);
    int code = http.GET();

    if (code != HTTP_CODE_OK) {
        http.end();
        if (g_last_price.price_section != SECTION_UNAVAILABLE) g_last_price.price_section = SECTION_STALE;
        return;
    }

    String payload = http.getString();
    http.end();

    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return;

    JsonObject btc = doc["bitcoin"];
    if (btc.isNull()) return;

    float usd = btc["usd"].as<float>();
    g_last_price.value_usd = usd;
    g_last_price.price_section = SECTION_OK;
}
