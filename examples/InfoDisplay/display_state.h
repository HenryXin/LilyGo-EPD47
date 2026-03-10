/**
 * Display state and last-value storage for InfoDisplay.
 * Supports Loading, Full, Partial, Offline per spec.
 */
#ifndef DISPLAY_STATE_H
#define DISPLAY_STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DISPLAY_LOADING,
    DISPLAY_FULL,
    DISPLAY_PARTIAL,
    DISPLAY_OFFLINE
} DisplayState_t;

typedef enum {
    SECTION_OK,
    SECTION_STALE,
    SECTION_UNAVAILABLE
} SectionState_t;

typedef struct {
    char     date_str[32];
    char     time_str[16];
    SectionState_t time_section;
} LastTime_t;

typedef struct {
    char     condition[32];
    float   temperature;
    SectionState_t weather_section;
} LastWeather_t;

typedef struct {
    float   value_usd;
    SectionState_t price_section;
} LastPrice_t;

#define LOGSEQ_MAX_ITEMS 10
#define LOGSEQ_TITLE_LEN 48
typedef struct {
    char    title[LOGSEQ_TITLE_LEN];
    bool    done;   /* true = DONE, false = TODO/pending */
} LogseqTodoItem_t;

typedef struct {
    LogseqTodoItem_t items[LOGSEQ_MAX_ITEMS];
    int              count;
    SectionState_t   section;
} LastLogseq_t;

extern DisplayState_t g_display_state;
extern LastTime_t     g_last_time;
extern LastWeather_t g_last_weather;
extern LastPrice_t   g_last_price;
extern LastLogseq_t  g_last_logseq;

void display_state_init(void);

#endif
