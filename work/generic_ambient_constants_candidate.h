#pragma once
#include "generic_ambient_state_candidate.h"
#include "generic_ambient_expansion_candidate.h"
#include "d3d9_shader_constants_candidate.h"
#include "native_shader_command_plan_candidate.h"
// Metadata-bound CPU upload plan using the recovered 10012140 RGB expansion.
// Full command execution and scene lighting initialization remain unverified.
inline NativeShaderCommandPlanCandidate genericAmbientConstantsCandidate(
    const std::vector<uint8_t>& vertexShader,const GenericAmbientStateCandidate& ambient) {
    const auto constants=d3d9ShaderConstantsCandidate(vertexShader);
    NativeShaderCommandPlanCandidate plan;plan.selections={0};
    const auto expanded=genericAmbientExpansionCandidate(ambient.packed);
    const char* names[]={"cAmbientCubeX","cAmbientCubeY","cAmbientCubeZ"};
    for(unsigned axis=0;axis<3;++axis){
        const D3d9ShaderConstantCandidate* found=nullptr;
        for(const auto& entry:constants)if(entry.name==names[axis])found=&entry;
        if(!found||found->registerSet!=2||found->count!=2)
            throw std::runtime_error("generic ambient constant metadata");
        std::vector<uint32_t> words(expanded.begin()+axis*8,expanded.begin()+axis*8+8);
        plan.uploads.push_back({false,found->index,std::move(words)});
    }
    return plan;
}
