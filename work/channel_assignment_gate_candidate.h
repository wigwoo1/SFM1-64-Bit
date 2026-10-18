#pragma once
#include <cstdint>

// Recovered 10366000/103623b0 ordering. Callbacks must implement the original
// graph operations; this portable candidate is not a production graph adapter.
template<class Resolve, class IsType, class GetMode, class BeforeEdit, class MarkOwner>
bool channelAssignmentGateCandidate(std::uint16_t& flags,
    std::uint32_t currentHandle, std::uint32_t incomingHandle,
    std::uint32_t expectedType, Resolve resolve, IsType isType,
    GetMode getMode, BeforeEdit beforeEdit, MarkOwner markOwner) {
    if ((flags & 0x1f) != 1 || currentHandle == incomingHandle) return false;
    if (expectedType != 0xffffffffu) {
        auto object = resolve(incomingHandle);
        if (object && !isType(object, expectedType)) return false;
    }
    if (flags & 0x20) return false;
    const auto mode = getMode();
    if (mode != 0 && mode != 1 && mode != 2 &&
        (mode != 4 || (flags & 0x400))) return false;
    if (!(flags & 0x80) && (flags & 0x2000)) beforeEdit();
    flags |= 0x4080;
    markOwner();
    return true;
}
