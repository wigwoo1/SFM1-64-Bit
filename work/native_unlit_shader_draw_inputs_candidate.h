#pragma once
#include "d3d9_shader_constants_candidate.h"
#include <array>
struct NativeUnlitConstantUploadCandidate {uint32_t index;std::vector<float> values;};
inline std::vector<NativeUnlitConstantUploadCandidate> nativeUnlitDiagnosticConstantsCandidate(const std::vector<uint8_t>& shader) {
    auto constants=d3d9ShaderConstantsCandidate(shader);
    std::vector<NativeUnlitConstantUploadCandidate> commands{{0,{0,1,0,0}}};
    auto append=[&](const char* name,std::vector<float> values){
        const D3d9ShaderConstantCandidate* found=nullptr;
        for(const auto& constant:constants)if(constant.name==name)found=&constant;
        if(!found||found->registerSet!=2||size_t(found->count)*4!=values.size())
            throw std::runtime_error("diagnostic UnlitGeneric constant layout mismatch");
        commands.push_back({found->index,std::move(values)});
    };
    append("cViewProj",{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1});
    append("cViewProjZ",{0,0,1,0});
    append("cModulationColor",{1,128.f/255.f,64.f/255.f,1});
    append("cBaseTextureTransform",{1,0,0,0,0,1,0,0});
    append("cDetailTextureTransform",{1,0,0,0,0,1,0,0});
    append("cModel",{1,0,0,0,0,1,0,0,0,0,1,0});
    return commands;
}
