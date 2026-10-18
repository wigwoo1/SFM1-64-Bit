#pragma once
#include <array>
#include <cstdint>
#include <cstring>
inline uint32_t recoveredPackColorCandidate(const std::array<float,4>&values){uint32_t packed=0;for(unsigned i=0;i<4;++i){float multiplier;uint32_t bits=0x437fffbeu;std::memcpy(&multiplier,&bits,4);float value=values[i]*multiplier;if(value>255.0f)value=255.0f;else if(value<0.0f)value=0.0f;uint32_t byte=value!=value?0u:uint32_t(value);packed|=(byte&255u)<<(i*8);}return packed;}
