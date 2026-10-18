#pragma once
#include "generic_lighting_draw_candidate.h"
#include "generic_lighting_upload_bundle_candidate.h"
#include "native_shader_program_selection_candidate.h"
struct GenericPreparedSingleDrawCandidate {
    NativeShaderProgramSelectionCandidate::Programs programs;
    GenericLightingUploadBundleCandidate lighting;
    uint32_t vertexDynamic=0,pixelDynamic=0;
};
// Caller chooses static/material branch and water-write policy explicitly.
// Covers the recovered nonbump VS20/PS20 single-record path only.
inline GenericPreparedSingleDrawCandidate genericSingleDrawPreparationCandidate(
    NativeShaderProgramSelectionCandidate& selection,
    const GenericAmbientStateCandidate& ambient,const GenericLightArrayCandidate& lights,
    const std::array<uint32_t,18>& ambientBaseline,uint32_t compression,int32_t bones,
    uint8_t staticControlFlow,int32_t constructorQuery,bool waterWrite,
    int32_t configuredCount,bool skipUnusedFill,const std::vector<uint32_t>& unusedWords,
    const GenericLightingConstantsRecordCandidate& previous){
    const auto state=genericAggregateLightStateCandidate({genericSingleLightRecordCandidate(ambient,lights)},ambientBaseline);
    GenericPreparedSingleDrawCandidate result;
    result.vertexDynamic=genericVs20DynamicComboCheckedCandidate(genericLightingDynamicFieldsCandidate(state,compression,bones,staticControlFlow,constructorQuery));
    result.pixelDynamic=genericPs20DynamicComboCandidate(waterWrite,constructorQuery);
    const uint32_t commands[]{12,result.vertexDynamic,11,result.pixelDynamic,0};
    result.programs=selection.executePrepared(commands,5,[&](const auto& programs){
        result.lighting=genericLightingUploadBundleCandidate(*programs.vertex,ambient,lights,configuredCount,skipUnusedFill,unusedWords,previous);
    });
    return result;
}
