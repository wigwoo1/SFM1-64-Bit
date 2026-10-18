#pragma once
#include <cstddef>
#include <cstdint>

// Portable reconstruction of10356870, not the original x86 manager layout.
// Entry/object identity and lifecycle must be supplied by the recovered manager.
struct ElementOwnerCounterCandidate { std::uint32_t count = 0; };
struct ElementOwnerPoolEntryCandidate {
    std::uint32_t generationAndFlags = 0;
    ElementOwnerCounterCandidate* object = nullptr;
};
inline ElementOwnerCounterCandidate* resolveElementOwnerCandidate(
    std::uint32_t ownerHandle,
    const ElementOwnerPoolEntryCandidate* entries, std::size_t count) {
    if (ownerHandle == 0xffffffffu) return nullptr;
    const auto index = ownerHandle & 0xfffffu;
    if (index >= count) return nullptr;
    // As in the original, nonnull table is a precondition for in-range access.
    const auto& entry = entries[index];
    const auto generation = (ownerHandle >> 20) & 0x7ffu;
    if (((generation ^ entry.generationAndFlags) & 0x7fffffffu) != 0) return nullptr;
    if ((entry.generationAndFlags & 0x80000000u) != 0) return nullptr;
    return entry.object;
}
inline void registerElementOwnerCandidate(
    std::uint32_t /*elementHandleUnused*/, std::uint32_t ownerHandle,
    const ElementOwnerPoolEntryCandidate* entries, std::size_t count) {
    if (auto* owner = resolveElementOwnerCandidate(ownerHandle,entries,count))
        ++owner->count;
}
//103567b0 returns zero for unresolved owners, otherwise the raw uint32 count.
inline std::uint32_t elementOwnerCountCandidate(std::uint32_t ownerHandle,
    const ElementOwnerPoolEntryCandidate* entries,std::size_t count) {
    auto* owner=resolveElementOwnerCandidate(ownerHandle,entries,count);
    return owner ? owner->count : 0;
}
//10356810 shares the same validation; decrement is unconditional after resolve.
inline void unregisterElementOwnerCandidate(
    std::uint32_t /*elementHandleUnused*/, std::uint32_t ownerHandle,
    const ElementOwnerPoolEntryCandidate* entries, std::size_t count) {
    if (auto* owner = resolveElementOwnerCandidate(ownerHandle,entries,count))
        --owner->count;
}
// Direct phase3 branch of10361eb0. Other phases require graph/undo traversal.
inline void transitionElementOwnerPhase3Candidate(
    std::uint32_t elementHandle, std::uint32_t& ownerHandle,
    std::uint32_t newOwnerHandle,
    const ElementOwnerPoolEntryCandidate* entries, std::size_t count) {
    unregisterElementOwnerCandidate(elementHandle,ownerHandle,entries,count);
    ownerHandle = newOwnerHandle;
    registerElementOwnerCandidate(elementHandle,ownerHandle,entries,count);
}
