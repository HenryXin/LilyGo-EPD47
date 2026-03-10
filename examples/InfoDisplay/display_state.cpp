#include "display_state.h"

DisplayState_t g_display_state = DISPLAY_LOADING;
LastTime_t     g_last_time;
LastWeather_t  g_last_weather;
LastPrice_t    g_last_price;
LastLogseq_t   g_last_logseq;

void display_state_init(void) {
    g_display_state = DISPLAY_LOADING;
    g_last_time.date_str[0] = '\0';
    g_last_time.time_str[0] = '\0';
    g_last_time.time_section = SECTION_UNAVAILABLE;
    g_last_weather.condition[0] = '\0';
    g_last_weather.temperature = 0.f;
    g_last_weather.weather_section = SECTION_UNAVAILABLE;
    g_last_price.value_usd = 0.f;
    g_last_price.price_section = SECTION_UNAVAILABLE;
    g_last_logseq.count = 0;
    g_last_logseq.section = SECTION_UNAVAILABLE;
}
