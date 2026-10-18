#pragma once
#include <d3d9.h>
#include <cstring>
#include "native_shader_command_plan_candidate.h"
#include "retained_shader_constants_candidate.h"
inline void nativeD3d9ShaderConstantsCandidate(IDirect3DDevice9* device,
    const NativeShaderCommandPlanCandidate& plan,uint32_t vertexLimit,uint32_t pixelLimit) {
    struct Prepared {bool pixel;uint32_t index;std::vector<float> values;};
    std::vector<Prepared> prepared;
    for(const auto& upload:plan.uploads){auto limit=upload.pixel?pixelLimit:vertexLimit;
        if(upload.words.size()%4||upload.index>limit||upload.words.size()/4>limit-upload.index)
            throw std::runtime_error("D3D9 constant plan range");
        Prepared entry{upload.pixel,upload.index,std::vector<float>(upload.words.size())};
        if(!entry.values.empty())std::memcpy(entry.values.data(),upload.words.data(),upload.words.size()*4);
        prepared.push_back(std::move(entry));
    }
    if(!device)throw std::runtime_error("D3D9 constant device missing");
    for(const auto& upload:prepared){if(upload.values.empty())continue;
        auto count=static_cast<UINT>(upload.values.size()/4);
        auto result=upload.pixel?device->SetPixelShaderConstantF(upload.index,upload.values.data(),count):
            device->SetVertexShaderConstantF(upload.index,upload.values.data(),count);
        if(FAILED(result))throw std::runtime_error("D3D9 constant upload failed");
        std::vector<float> readback(upload.values.size());
        result=upload.pixel?device->GetPixelShaderConstantF(upload.index,readback.data(),count):
            device->GetVertexShaderConstantF(upload.index,readback.data(),count);
        if(FAILED(result)||std::memcmp(readback.data(),upload.values.data(),upload.values.size()*sizeof(float)))
            throw std::runtime_error("D3D9 constant register readback mismatch");
    }
}

// One owner per device; callers invalidate after reset or external register writes.
class RetainedNativeD3d9ShaderConstantsCandidate {
    IDirect3DDevice9* device_;
    uint32_t vertexLimit_,pixelLimit_;
    RetainedShaderConstantPlanCandidate cache_;
public:
    RetainedNativeD3d9ShaderConstantsCandidate(IDirect3DDevice9* device,uint32_t vertexLimit,uint32_t pixelLimit):
        device_(device),vertexLimit_(vertexLimit),pixelLimit_(pixelLimit),cache_(vertexLimit,pixelLimit){
        if(!device_)throw std::runtime_error("retained D3D9 constant device missing");device_->AddRef();
    }
    ~RetainedNativeD3d9ShaderConstantsCandidate(){device_->Release();}
    RetainedNativeD3d9ShaderConstantsCandidate(const RetainedNativeD3d9ShaderConstantsCandidate&)=delete;
    RetainedNativeD3d9ShaderConstantsCandidate& operator=(const RetainedNativeD3d9ShaderConstantsCandidate&)=delete;
    void invalidate(){cache_.invalidate();}
    size_t apply(const NativeShaderCommandPlanCandidate& plan){return cache_.apply(plan,[&](const NativeShaderConstantWordsCandidate& command){
        NativeShaderCommandPlanCandidate single;single.uploads.push_back(command);
        nativeD3d9ShaderConstantsCandidate(device_,single,vertexLimit_,pixelLimit_);
    });}
};
