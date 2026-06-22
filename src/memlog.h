#ifndef MEMLOG_H
#define MEMLOG_H

#include <Arduino.h>

enum MemLogType : uint8_t
{
    MEMLOG_CHECKIN = 1,
    MEMLOG_ERROR = 2,
    MEMLOG_OTA = 3,
    MEMLOG_SOI = 4,
    MEMLOG_SHT = 5,
    MEMLOG_TASK = 6,
};

void memlogSetSlots(uint8_t n);
uint8_t memlogMaxSlots();
size_t memlogJsonCapacity();
void memlogAdd(MemLogType type, int code, const char *msg);
void memlogAdd(MemLogType type, int code, const String &msg);
void memlogTask(const char *name, int code = 0);
void memlogClear();
uint8_t memlogCount();
size_t memlogWriteJson(char *buf, size_t cap, const char *fwVersion = "");

#endif
