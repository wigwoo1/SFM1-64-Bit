#pragma once
#include <cstdint>
#include <cstring>
// 1097bc10; sequential loads/stores preserve original partial-overlap behavior.
inline void copyElementFixed16Candidate(const void* source,void* destination) {
    const auto* src=static_cast<const unsigned char*>(source);
    auto* dst=static_cast<unsigned char*>(destination);
    std::uint64_t chunk;
    std::memcpy(&chunk,src,8);std::memcpy(dst,&chunk,8);
    std::memcpy(&chunk,src+8,8);std::memcpy(dst+8,&chunk,8);
}
