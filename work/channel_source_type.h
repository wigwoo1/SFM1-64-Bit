#pragma once
#include <cstdint>
// Source attribute flags are masked to five bits; log type is already raw.
inline uint32_t channelScalarType(uint32_t type){return type>=15&&type<=28?type-14:type;}
inline uint32_t channelSourceScalarType(uint16_t flags){return channelScalarType(flags&0x1f);}
