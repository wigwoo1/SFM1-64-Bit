#pragma once
#include "material_shader_texture_init_candidate.h"
#include "material_texture_file_resolver_candidate.h"
#include "material_texture_converted_image_candidate.h"

// Pending cold normal-texture lookup data, not a completed manager/cache entry
// or MaterialShaderTextureLookupCandidate. It has no implicit typed/GPU result.
class ShaderTextureCpuPreparationCandidate {
    std::string key;
    std::optional<std::string>group;
    uint32_t requestFlags=0;
    std::shared_ptr<const MaterialTextureResourceCandidate>file;
    std::shared_ptr<const MaterialTextureConvertedImageCandidate>prepared;
    ShaderTextureCpuPreparationCandidate()=default;
public:
    // Caller has established a cold ordinary normal-texture miss and enabled
    // Download gates. This function neither looks up nor commits a native cache.
    // supplied is an authorized read-only snapshot for this exact logical key.
    static bool prepareColdNormalMiss(const MaterialShaderTextureRequestCandidate&request,
        const MaterialTextureReferenceCandidate&supplied,
        MaterialTextureCpuImageSettingsCandidate settings,uint32_t compressionConfig,
        const TextureFormatPolicyServicesCandidate&services,
        std::shared_ptr<const ShaderTextureCpuPreparationCandidate>&output,
        size_t maxOutputBytes=size_t(1)<<29)noexcept{
        try{
            const auto r=request;const auto ref=supplied;
            if(r.kind!=MaterialShaderTextureKindCandidate::Normal||!r.group||r.group->size()>255||r.group->find('\0')!=std::string::npos)return false;
            for(unsigned char c:*r.group)if(c>=128)return false;
            MaterialTextureFileRequestCandidate logical;if(!materialTextureFileRequestCandidate(r.name,logical))return false;
            if(!ref.valid()||ref.isEnvironment()||!ref.resource()||ref.resource()->name()!=logical.key)return false;
            settings.requestedFlags=r.flags;
            std::shared_ptr<const MaterialTextureCpuImageCandidate>image;
            if(!MaterialTextureCpuImageCandidate::prepare(ref,settings,image))return false;
            std::shared_ptr<const MaterialTextureConvertedImageCandidate>converted;
            if(!MaterialTextureConvertedImageCandidate::prepare(image,compressionConfig,services,converted,maxOutputBytes))return false;
            auto next=std::shared_ptr<ShaderTextureCpuPreparationCandidate>(new ShaderTextureCpuPreparationCandidate);
            next->key=std::move(logical.key);next->group=r.group;next->requestFlags=r.flags;next->file=ref.resource();next->prepared=std::move(converted);output=std::move(next);return true;
        }catch(...){return false;}
    }
    const std::string&name()const noexcept{return key;}
    const std::optional<std::string>&textureGroup()const noexcept{return group;}
    uint32_t requestedFlags()const noexcept{return requestFlags;}
    const std::shared_ptr<const MaterialTextureResourceCandidate>&resource()const noexcept{return file;}
    const std::shared_ptr<const MaterialTextureConvertedImageCandidate>&image()const noexcept{return prepared;}
    bool hasAlpha()const noexcept{return (prepared->plan()[0]&0x3000u)!=0;}
};
