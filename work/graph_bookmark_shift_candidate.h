#pragma once
#include <cstdint>
#include <cstring>
inline void* graphBookmarkShiftCandidate(void* destination,const void* source,std::uint32_t bytes){return std::memmove(destination,source,bytes);}
