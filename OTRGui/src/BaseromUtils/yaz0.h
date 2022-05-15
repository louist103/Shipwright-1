#pragma once

#ifdef __cplusplus
#include <vector>
std::vector<uint8_t> yaz0_encode(const uint8_t* src, int src_size);
extern "C" {
#endif

void yaz0_decode(const uint8_t* src, uint8_t* dest, size_t destsize);

#ifdef __cplusplus
}
#endif