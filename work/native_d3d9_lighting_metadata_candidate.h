#pragma once
#include <d3d9.h>
#include <cstring>
#include "generic_lighting_metadata_candidate.h"
inline void nativeD3d9LightingMetadataCandidate(IDirect3DDevice9* device,
    const std::vector<VertexMetadataUploadCandidate>& plan,uint32_t integerLimit,uint32_t booleanLimit){
    // Use the same owned validation before touching the device. Unknown cache
    // state ensures all requests, including forced ones, reach the callback.
    RetainedVertexMetadataCandidate validator(integerLimit,booleanLimit);
    validator.apply(plan,[&](const VertexMetadataUploadCandidate& command){
        if(!device)throw std::runtime_error("D3D9 metadata device missing");
        if(command.words.empty())return;
        HRESULT result;
        if(command.kind==VertexMetadataKindCandidate::Integer){
            std::vector<int> values(command.words.size()),readback(command.words.size());
            std::memcpy(values.data(),command.words.data(),values.size()*sizeof(int));
            const UINT count=static_cast<UINT>(values.size()/4);
            result=device->SetVertexShaderConstantI(command.index,values.data(),count);
            if(FAILED(result))throw std::runtime_error("D3D9 integer metadata upload failed");
            result=device->GetVertexShaderConstantI(command.index,readback.data(),count);
            if(FAILED(result)||std::memcmp(values.data(),readback.data(),values.size()*sizeof(int)))throw std::runtime_error("D3D9 integer metadata readback mismatch");
        }else{
            std::vector<BOOL> values(command.words.size()),readback(command.words.size());
            std::memcpy(values.data(),command.words.data(),values.size()*sizeof(BOOL));
            const UINT count=static_cast<UINT>(values.size());
            result=device->SetVertexShaderConstantB(command.index,values.data(),count);
            if(FAILED(result))throw std::runtime_error("D3D9 boolean metadata upload failed");
            result=device->GetVertexShaderConstantB(command.index,readback.data(),count);
            if(FAILED(result)||std::memcmp(values.data(),readback.data(),values.size()*sizeof(BOOL)))throw std::runtime_error("D3D9 boolean metadata readback mismatch");
        }
    });
}
class RetainedNativeD3d9LightingMetadataCandidate {
    IDirect3DDevice9* device_;
    uint32_t integerLimit_,booleanLimit_;
    RetainedVertexMetadataCandidate cache_;
public:
    RetainedNativeD3d9LightingMetadataCandidate(IDirect3DDevice9* device,uint32_t integers,uint32_t booleans):
        device_(device),integerLimit_(integers),booleanLimit_(booleans),cache_(integers,booleans){
        if(!device_)throw std::runtime_error("retained D3D9 metadata device missing");device_->AddRef();
    }
    ~RetainedNativeD3d9LightingMetadataCandidate(){device_->Release();}
    RetainedNativeD3d9LightingMetadataCandidate(const RetainedNativeD3d9LightingMetadataCandidate&)=delete;
    RetainedNativeD3d9LightingMetadataCandidate& operator=(const RetainedNativeD3d9LightingMetadataCandidate&)=delete;
    void invalidate(){cache_.invalidate();}
    size_t apply(const std::vector<VertexMetadataUploadCandidate>& plan){
        return cache_.apply(plan,[&](const auto& command){nativeD3d9LightingMetadataCandidate(device_,{command},integerLimit_,booleanLimit_);});
    }
};
