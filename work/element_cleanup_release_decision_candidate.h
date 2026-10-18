#pragma once
#include <cstdint>
//1035b384..1035b3b5 decision only; record field meanings remain unresolved.
inline bool elementCleanupImmediateReleaseCandidate(std::uint32_t mode,
    std::uint32_t recordFieldC,std::uint32_t recordField4) {
    return mode==0 || (mode==2 && recordFieldC==0xffffffffu &&
        (recordField4&0x3fffffffu)==0);
}
