#include "display_state.h"
#include "config.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>
#include <cstring>

#ifndef LOGSEQ_TODOS_URL
#define LOGSEQ_TODOS_URL ""
#endif

static bool logseq_configured(void) {
    return strlen(LOGSEQ_SERVER_URL) > 0 && strlen(LOGSEQ_API_TOKEN) > 0;
}

/* Fetch today's todos from optional bridge (reads journal files). Returns count or -1. */
static int fetch_via_bridge(const char *date_yyyy_mm_dd) {
    if (strlen(LOGSEQ_TODOS_URL) == 0) return -1;
    String url = String(LOGSEQ_TODOS_URL) + "?date=" + date_yyyy_mm_dd;
    HTTPClient http;
    if (!http.begin(url)) return -1;
    http.setTimeout(8000);
    int code = http.GET();
    String payload = http.getString();
    http.end();
    if (code != 200 || payload.length() == 0) return -1;
    StaticJsonDocument<2048> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return -1;
    JsonArray arr = doc.as<JsonArray>();
    if (arr.isNull()) return -1;
    int count = 0;
    for (JsonObject obj : arr) {
        if (count >= LOGSEQ_MAX_ITEMS) break;
        const char *title = obj["title"].as<const char *>();
        bool done = obj["done"].as<bool>();
        if (title) {
            strncpy(g_last_logseq.items[count].title, title, LOGSEQ_TITLE_LEN - 1);
            g_last_logseq.items[count].title[LOGSEQ_TITLE_LEN - 1] = '\0';
        } else
            g_last_logseq.items[count].title[0] = '\0';
        g_last_logseq.items[count].done = done;
        count++;
    }
    g_last_logseq.count = count;
    return count;
}

/* Recursively collect blocks with marker TODO or DONE into g_last_logseq. */
static void collect_todos_from_json(JsonVariant node, int *out_count) {
    if (node.is<JsonObject>()) {
        JsonObject obj = node.as<JsonObject>();
        const char *marker = obj["marker"].as<const char *>();
        const char *content = obj["content"].as<const char *>();
        bool is_todo = marker && (strcmp(marker, "TODO") == 0);
        bool is_done = marker && (strcmp(marker, "DONE") == 0);
        /* Fallback: content may start with "TODO " or "DONE " (Logseq/Org style) */
        if (!is_todo && !is_done && content) {
            if (strncmp(content, "TODO ", 5) == 0) { is_todo = true; content += 5; }
            else if (strncmp(content, "DONE ", 5) == 0) { is_done = true; content += 5; }
        }
        if ((is_todo || is_done) && !content)
            content = "";
        if ((is_todo || is_done) && *out_count < LOGSEQ_MAX_ITEMS) {
            bool done = is_done;
            if (content && *content) {
                strncpy(g_last_logseq.items[*out_count].title, content, LOGSEQ_TITLE_LEN - 1);
                g_last_logseq.items[*out_count].title[LOGSEQ_TITLE_LEN - 1] = '\0';
            } else {
                g_last_logseq.items[*out_count].title[0] = '\0';
            }
            g_last_logseq.items[*out_count].done = done;
            (*out_count)++;
        }
        if (obj.containsKey("children")) {
            for (JsonVariant c : obj["children"].as<JsonArray>())
                collect_todos_from_json(c, out_count);
        }
        return;
    }
    if (node.is<JsonArray>()) {
        for (JsonVariant v : node.as<JsonArray>())
            collect_todos_from_json(v, out_count);
    }
}

/* POST a single method with args[]; return 200 + payload, or non-200 / -1 on failure. */
static int api_call(HTTPClient &http, const char *method, const char *arg_page, String *out_payload) {
    StaticJsonDocument<192> reqDoc;
    reqDoc["method"] = method;
    JsonArray args = reqDoc.createNestedArray("args");
    if (arg_page) args.add(arg_page);
    String reqBody;
    serializeJson(reqDoc, reqBody);
    int code = http.POST(reqBody);
    *out_payload = http.getString();
    return code;
}

/* Parse getPageBlocksTree / getCurrentPageBlocksTree result into g_last_logseq; return count. */
static int parse_blocks_into_logseq(const String &payload) {
    StaticJsonDocument<2048> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return -2;
    JsonVariant root = doc["result"];
    if (root.isNull()) root = doc.as<JsonVariant>();
    int count = 0;
    collect_todos_from_json(root, &count);
    g_last_logseq.count = count;
    return count;
}

/* Return true if page name matches today (case-insensitive; accepts yyyy_MM_dd or yyyy-MM-dd). */
static bool page_is_today(const char *name, const char *today_underscore, const char *today_dash) {
    if (!name || !*name) return false;
    return (strcasecmp(name, today_underscore) == 0) || (strcasecmp(name, today_dash) == 0);
}

void logseq_fetch(void) {
    if (!logseq_configured()) {
        g_last_logseq.section = SECTION_UNAVAILABLE;
        g_last_logseq.count = 0;
        return;
    }
    if (WiFi.status() != WL_CONNECTED) {
        if (g_last_logseq.section != SECTION_UNAVAILABLE) g_last_logseq.section = SECTION_STALE;
        Serial.println("[Logseq] WiFi not connected");
        return;
    }

    struct tm t;
    if (!getLocalTime(&t)) {
        if (g_last_logseq.section != SECTION_UNAVAILABLE) g_last_logseq.section = SECTION_STALE;
        Serial.println("[Logseq] No local time (NTP?), cannot build today page name");
        return;
    }
    char page_name[16], page_alt[16];
    strftime(page_name, sizeof(page_name), "%Y_%m_%d", &t);
    strftime(page_alt, sizeof(page_alt), "%Y-%m-%d", &t);

    /* Optional bridge: reads journal files on PC (works around getPageBlocksTree returning null). */
    int count = fetch_via_bridge(page_name);
    if (count >= 0) {
        g_last_logseq.section = SECTION_OK;
        Serial.printf("[Logseq] bridge OK %d item(s)\n", count);
        return;
    }

    String url = String(LOGSEQ_SERVER_URL) + "/api";
    HTTPClient http;
    if (!http.begin(url)) {
        Serial.printf("[Logseq] http.begin failed for %s\n", url.c_str());
        if (g_last_logseq.section != SECTION_UNAVAILABLE) g_last_logseq.section = SECTION_STALE;
        return;
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + LOGSEQ_API_TOKEN);
    http.setTimeout(10000);

    String payload;
    count = 0;

    /* Workaround for getPageBlocksTree bug (returns null): if current page is today's journal,
       use getCurrentPageBlocksTree instead. */
    int code = api_call(http, "logseq.Editor.getCurrentPage", nullptr, &payload);
    if (code == 200 && payload.length() > 0) {
        StaticJsonDocument<384> doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            JsonVariant r = doc["result"];
            const char *cur = nullptr;
            if (!r.isNull() && r.is<JsonObject>()) {
                JsonObject res = r.as<JsonObject>();
                cur = res["name"].as<const char *>();
                if (!cur || !cur[0]) cur = res["originalName"].as<const char *>();
                if (!cur || !cur[0]) cur = res["title"].as<const char *>();
                if ((!cur || !cur[0]) && res.containsKey("page")) cur = res["page"]["name"].as<const char *>();
            }
            if (cur && cur[0])
                Serial.printf("[Logseq] current page: \"%s\" (today=%s)\n", cur, page_is_today(cur, page_name, page_alt) ? "yes" : "no");
            else {
                Serial.println("[Logseq] current page: (none or empty)");
                unsigned len = payload.length() < 280 ? payload.length() : 280;
                Serial.printf("[Logseq] getCurrentPage response (%u): ", (unsigned)payload.length());
                for (unsigned i = 0; i < len; i++) Serial.print(payload[i]);
                Serial.println();
            }
            if (page_is_today(cur, page_name, page_alt)) {
                code = api_call(http, "logseq.Editor.getCurrentPageBlocksTree", nullptr, &payload);
                if (code == 200) {
                    count = parse_blocks_into_logseq(payload);
                    if (count >= 0) {
                        Serial.printf("[Logseq] current page is today (%s), got %d item(s)\n", cur, count);
                        http.end();
                        g_last_logseq.section = SECTION_OK;
                        Serial.printf("[Logseq] OK %d item(s)\n", count);
                        return;
                    }
                }
            }
        } else
            Serial.println("[Logseq] getCurrentPage: parse error");
    } else
        Serial.printf("[Logseq] getCurrentPage: HTTP %d or empty\n", code);

    /* Fallback: getPageBlocksTree (known to return null in many setups). */
    Serial.printf("[Logseq] POST %s page=%s\n", url.c_str(), page_name);
    code = api_call(http, "logseq.Editor.getPageBlocksTree", page_name, &payload);
    if (code != 200) count = -1;
    else count = parse_blocks_into_logseq(payload);
    if (count == 0 && strcmp(page_name, page_alt) != 0) {
        Serial.printf("[Logseq] 0 with %s, retry page=%s\n", page_name, page_alt);
        code = api_call(http, "logseq.Editor.getPageBlocksTree", page_alt, &payload);
        if (code == 200) count = parse_blocks_into_logseq(payload);
    }
    http.end();

    if (count < 0) {
        if (count == -1) Serial.println("[Logseq] HTTP not 200");
        else Serial.printf("[Logseq] JSON parse error (payload len=%u)\n", (unsigned)payload.length());
        if (g_last_logseq.section != SECTION_UNAVAILABLE) g_last_logseq.section = SECTION_STALE;
        return;
    }

    if (count == 0) {
        Serial.println("[Logseq] getPageBlocksTree returned empty (known API bug). Open today's journal in Logseq to sync.");
    }

    g_last_logseq.section = SECTION_OK;
    Serial.printf("[Logseq] OK %d item(s)\n", count);
}
