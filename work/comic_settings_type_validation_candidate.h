#pragma once
#include "base_element_type_validation_candidate.h"
// 1000cb10: CDmeComicSettings vslot4; not a universal derived-class policy.
inline bool comicSettingsAcceptsTypeCandidate(std::uintptr_t requested,std::uintptr_t ownToken,std::uintptr_t baseToken) {
    return requested==ownToken || baseElementAcceptsTypeCandidate(requested,baseToken);
}
