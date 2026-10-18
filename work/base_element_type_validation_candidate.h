#pragma once
#include <cstdint>
#include <limits>
// 1035db00: base CDmElement vslot4. Derived implementations remain separate.
inline bool baseElementAcceptsTypeCandidate(std::uintptr_t requested,std::uintptr_t registeredBaseToken) {
    return requested==registeredBaseToken || requested==std::numeric_limits<std::uintptr_t>::max();
}
