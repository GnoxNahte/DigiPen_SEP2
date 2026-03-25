#pragma once
#include <cstdint>

struct SaveData
{
    uint32_t version = 1;
    int32_t levelId = 0;
    int32_t hp = 100;
    float totalSeconds = 0.f;
};
