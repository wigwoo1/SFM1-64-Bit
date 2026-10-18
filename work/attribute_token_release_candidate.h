#pragma once
#include <cstdint>
#include <cstddef>
struct AttributeTokenSlotCandidate {
    std::uint32_t state = 0;
    void* attribute = nullptr;
};
// Portable fields, not original byte layout. Enqueue callback supplies recovered
// ring operation; caller must preserve the original lifetime invariants.
template<class Enqueue>
void releaseAttributeTokenSlotCandidate(std::uint32_t token,
    AttributeTokenSlotCandidate* slots, std::uint32_t slotCount,
    std::uint32_t& liveCount, Enqueue enqueue) {
    const auto index = token & 0x3fffffu;
    if (index >= slotCount) return;
    auto& slot = slots[index];
    slot.state = (slot.state & 0x80000000u) | ((slot.state + 1u) & 0x7fffffffu);
    if (!(slot.state & 0x80000000u)) {
        slot.state |= 0x80000000u;
        --liveCount;
    }
    slot.attribute = nullptr;
    if ((slot.state & 0x7fffffffu) < 0x1ffu) enqueue(index);
}
