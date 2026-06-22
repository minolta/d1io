#include "memlog.h"
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string.h>

#define MEMLOG_MSG 28
#define MEMLOG_SLOTS_MIN 4
#define MEMLOG_SLOTS_MAX 48
#define MEMLOG_SLOTS_DEFAULT 16

struct MemLogEntry
{
    uint32_t uptime;
    uint8_t type;
    int16_t code;
    char msg[MEMLOG_MSG];
};

static MemLogEntry *_log = nullptr;
static uint8_t _maxSlots = MEMLOG_SLOTS_DEFAULT;
static uint8_t _idx = 0;
static uint8_t _count = 0;

static void memlogEnsure()
{
    if (_log == nullptr)
        memlogSetSlots(MEMLOG_SLOTS_DEFAULT);
}

static void copyMsg(char *dst, const char *src)
{
    if (src == nullptr)
        src = "";
    strncpy(dst, src, MEMLOG_MSG - 1);
    dst[MEMLOG_MSG - 1] = '\0';
}

void memlogSetSlots(uint8_t n)
{
    if (n < MEMLOG_SLOTS_MIN)
        n = MEMLOG_SLOTS_MIN;
    if (n > MEMLOG_SLOTS_MAX)
        n = MEMLOG_SLOTS_MAX;
    if (_log != nullptr && _maxSlots == n)
        return;
    if (_log != nullptr)
        free(_log);
    _maxSlots = n;
    _log = (MemLogEntry *)malloc(_maxSlots * sizeof(MemLogEntry));
    _idx = 0;
    _count = 0;
}

uint8_t memlogMaxSlots()
{
    memlogEnsure();
    return _maxSlots;
}

size_t memlogJsonCapacity()
{
    memlogEnsure();
    return 144 + (size_t)_maxSlots * 96;
}

void memlogAdd(MemLogType type, int code, const char *msg)
{
    memlogEnsure();
    if (_log == nullptr)
        return;
    MemLogEntry &e = _log[_idx];
    e.uptime = millis() / 1000;
    e.type = type;
    e.code = (int16_t)code;
    copyMsg(e.msg, msg);
    _idx = (_idx + 1) % _maxSlots;
    if (_count < _maxSlots)
        _count++;
}

void memlogAdd(MemLogType type, int code, const String &msg)
{
    memlogAdd(type, code, msg.c_str());
}

void memlogTask(const char *name, int code)
{
    memlogAdd(MEMLOG_TASK, code, name);
}

void memlogClear()
{
    _idx = 0;
    _count = 0;
}

uint8_t memlogCount()
{
    return _count;
}

static const char *memlogTypeName(uint8_t type)
{
    switch (type)
    {
    case MEMLOG_CHECKIN:
        return "checkin";
    case MEMLOG_ERROR:
        return "error";
    case MEMLOG_OTA:
        return "ota";
    case MEMLOG_SOI:
        return "soi";
    case MEMLOG_SHT:
        return "sht";
    case MEMLOG_TASK:
        return "task";
    default:
        return "?";
    }
}

size_t memlogWriteJson(char *buf, size_t cap, const char *fwVersion)
{
    memlogEnsure();
    if (_log == nullptr || buf == nullptr || cap < 48)
        return 0;

    if (fwVersion == nullptr)
        fwVersion = "";

    const size_t docSize = memlogJsonCapacity();
    DynamicJsonDocument doc(docSize);
    doc["version"] = fwVersion;
    doc["count"] = _count;
    doc["max"] = _maxSlots;
    doc["heap"] = ESP.getFreeHeap();
    JsonArray arr = doc.createNestedArray("logs");
    const uint8_t oldest = (_count < _maxSlots) ? 0 : _idx;

    for (uint8_t i = 0; i < _count; i++)
    {
        const MemLogEntry &e = _log[(oldest + i) % _maxSlots];
        JsonObject o = arr.createNestedObject();
        if (o.isNull())
            break;
        o["t"] = e.uptime;
        o["type"] = memlogTypeName(e.type);
        o["code"] = e.code;
        o["msg"] = e.msg;
    }

    const size_t n = serializeJson(doc, buf, cap);
    if (n > 0)
        return n;

    return (size_t)snprintf(buf, cap,
                            "{\"logs\":[],\"version\":\"%s\",\"count\":%u,\"max\":%u,\"heap\":%u}",
                            fwVersion, _count, _maxSlots, ESP.getFreeHeap());
}
