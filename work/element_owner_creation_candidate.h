#pragma once
#include <array>
#include <cstdint>
// Portable24-byte metadata values; original intern table/pool must be supplied.
struct ElementOwnerMetadataCandidate {
    std::uint32_t nameSymbol=0xffffffffu;
    std::uint32_t secondarySymbol=0xffffffffu;
    std::uint32_t rootHandle=0xffffffffu;
    std::uint8_t marked=1;
    std::uint32_t elementCount=0;
    std::uint32_t field14=0;
};
template<class Adapter>
std::uint32_t createElementOwnerCandidate(const char* name,Adapter& adapter) {
    if (!name || !*name) return 0xffffffffu;
    std::array<char,0x104> normalized; // Helper owns initialization/termination.
    adapter.normalize(normalized.data(),normalized.size(),name);
    auto existing=adapter.lookup(normalized.data());
    if (existing!=0xffffffffu) {
        adapter.mark(existing);
        return existing;
    }
    auto handle=adapter.reserve();
    auto* record=adapter.allocate();
    if (record) {
        record->nameSymbol=adapter.intern(normalized.data());
        record->secondarySymbol=0xffffffffu;
        record->rootHandle=0xffffffffu;
        record->marked=1;
        record->elementCount=0;
        record->field14=0;
    }
    adapter.bind(handle,record);
    return handle;
}
