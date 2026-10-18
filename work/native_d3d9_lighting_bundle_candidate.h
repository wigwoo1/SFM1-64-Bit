#pragma once
#include "native_d3d9_shader_constants_candidate.h"
#include "native_d3d9_lighting_metadata_candidate.h"
#include "generic_lighting_upload_bundle_candidate.h"
// Device render-owner only. No device rollback is promised after a partial
// upload failure; cache invalidation ensures the next attempt resends ranges.
class RetainedNativeD3d9LightingBundleCandidate {
    uint32_t vertexLimit_,pixelLimit_,integerLimit_,booleanLimit_;
    RetainedNativeD3d9ShaderConstantsCandidate floats_;
    RetainedNativeD3d9LightingMetadataCandidate metadata_;
public:
    struct Sent {size_t floats,metadata;};
    RetainedNativeD3d9LightingBundleCandidate(IDirect3DDevice9* device,uint32_t vertexLimit,
        uint32_t pixelLimit,uint32_t integerLimit,uint32_t booleanLimit):
        vertexLimit_(vertexLimit),pixelLimit_(pixelLimit),integerLimit_(integerLimit),booleanLimit_(booleanLimit),
        floats_(device,vertexLimit,pixelLimit),metadata_(device,integerLimit,booleanLimit){}
    void invalidate(){floats_.invalidate();metadata_.invalidate();}
    Sent apply(const GenericLightingUploadBundleCandidate& bundle){
        for(const auto& command:bundle.floats.uploads){
            const uint32_t limit=command.pixel?pixelLimit_:vertexLimit_;
            if(command.words.size()%4||command.index>limit||command.words.size()/4>limit-command.index)throw std::invalid_argument("lighting bundle float range");
        }
        for(const auto& command:bundle.metadata){
            if(command.kind!=VertexMetadataKindCandidate::Integer&&command.kind!=VertexMetadataKindCandidate::Boolean)throw std::invalid_argument("lighting bundle metadata kind");
            const size_t width=command.kind==VertexMetadataKindCandidate::Integer?4:1;
            const uint32_t limit=width==4?integerLimit_:booleanLimit_;
            if(command.words.size()%width||command.index>limit||command.words.size()/width>limit-command.index)throw std::invalid_argument("lighting bundle metadata range");
        }
        try{return {floats_.apply(bundle.floats),metadata_.apply(bundle.metadata)};}
        catch(...){invalidate();throw;}
    }
};
