#pragma once
#include <cstdint>
#include <cstring>
//1035a9d0 arithmetic uses32-bit wrap; normal string lengths must fit storage.
inline std::uint32_t ownerSymbolRecordBytesCandidate(std::uint32_t length) {
    return (length+8u)&0xfffffffcu;
}
inline std::uint32_t ownerSymbolNewBlockBytesCandidate(std::uint32_t recordBytes) {
    const auto size=recordBytes+12u;
    return size>2048u?size:2048u;
}
// Payload starts originalblock+8. Caller supplies first-fit capacity proof and
// stable/nonoverlapping input. Advances cursor before hash/string writes.
inline char* writeOwnerSymbolArenaRecordCandidate(char* payload,
    std::uint32_t& used,std::uint32_t recordBytes,std::uint32_t hash,
    const char* source,std::uint32_t stringBytesIncludingNul) {
    char* record=payload+used;
    used+=recordBytes;
    std::memcpy(record,&hash,4);
    std::memcpy(record+4,source,stringBytesIncludingNul);
    return record;
}
