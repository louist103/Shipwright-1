#ifndef ENDIAN_UTILS_H
#define ENDIAN_UTILS_H
#include <cstdio>

typedef enum class RomEndian {
    Unchecked,
    Big,
    Little,
    Half,
    Error
} RomEndian;

constexpr size_t ROM_54_MB = 0x3600000;
constexpr size_t ROM_64_MB = 0x4000000;
constexpr size_t ROM_32_MB = 0x2000000;

extern RomEndian GetRomEndian(FILE* romFile);
extern size_t gRomSize;

void* ConvertToBigEndianRam(FILE* romFile);

#endif