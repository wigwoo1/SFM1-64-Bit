#pragma once
#include "generic_ambient_state_candidate.h"
#include "generic_ambient_expansion_candidate.h"
#include "generic_light_array_candidate.h"
#include "generic_light_constants_candidate.h"
#include <algorithm>
// Owned representation of the recovered 0x224-byte constant record, not an
// x64 ABI layout. Retains unused light slots and unrelated record fields.
struct GenericLightingConstantsRecordCandidate {
    std::array<uint32_t,137> words{};
    void update(const GenericAmbientStateCandidate& ambient,const GenericLightArrayCandidate& lights){
        auto next=words;
        genericLightConstantsCandidate(next,lights.count,lights.words);
        const auto cube=genericAmbientExpansionCandidate(ambient.packed);
        std::copy(cube.begin(),cube.end(),next.begin());
        words=next;
    }
};
