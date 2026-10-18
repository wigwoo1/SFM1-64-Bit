#pragma once
#include "generic_ambient_state_candidate.h"
#include "generic_light_array_candidate.h"
#include "generic_light_state_candidate.h"
#include "generic_shader_combo_candidate.h"
// Connect recovered components for the identified single-record draw path.
// Caller still chooses the material branch and supplies hardware/model queries.
inline GenericLightRecordCandidate genericSingleLightRecordCandidate(
    const GenericAmbientStateCandidate& ambient,const GenericLightArrayCandidate& lights) {
    GenericLightRecordCandidate record;record.present=true;
    record.ambientWords=ambient.packed;record.lightCount=lights.count;
    return record; // Original builder leaves second-flag word zero.
}
inline std::array<uint32_t,6> genericLightingDynamicFieldsCandidate(
    const GenericLightStateCandidate& light,uint32_t compression,int32_t bones,
    uint8_t staticControlFlow,int32_t constructorQuery) {
    return genericVs20DynamicFieldsCandidate({compression,light.lightCount,
        static_cast<uint8_t>(light.firstFlag),bones,staticControlFlow,constructorQuery});
}
