#pragma once
#include "material_typed_dispatch_candidate.h"
#include <functional>
#include <type_traits>

enum class MaterialShaderTextureKindCandidate:uint32_t { Normal=0,Bump=1,Cube=2 };
struct MaterialShaderTextureRequestCandidate {
    MaterialShaderTextureKindCandidate kind=MaterialShaderTextureKindCandidate::Normal;
    std::string name;std::optional<std::string>group;uint32_t flags=0;
};
// Outer nullopt means unavailable service/state. A present result with no texture
// means an explicit original-manager null result, which requests error fallback.
// Neither is fabricated into a successful texture. Results retain real CPU bytes.
struct MaterialShaderTextureLookupCandidate {std::optional<MaterialTextureReferenceCandidate>texture;};
struct MaterialShaderTextureServicesCandidate {
    std::function<std::optional<uint32_t>()>configMask,hdrType,suppressWarning;
    std::function<std::optional<MaterialShaderTextureLookupCandidate>(const MaterialShaderTextureRequestCandidate&)>find;
    std::function<std::optional<MaterialTextureReferenceCandidate>()>errorTexture;
    std::function<void(const std::string&)>warning;
};
struct MaterialShaderTextureStateCandidate {MaterialTypedDispatchCandidate value;uint32_t flags2=0;};
// Opt-in CShaderSystem load policy, not the texture manager, VTF loading, graphics
// upload or production material init. Explicit immediate/ownerless CPU state only.
// Publish output only after successful preparation; input/output may alias.
inline bool materialShaderTextureInitCandidate(const MaterialShaderTextureStateCandidate&input,
    MaterialShaderTextureStateCandidate&output,MaterialShaderTextureKindCandidate kind,
    uint32_t flags,const std::optional<std::string>&group,const MaterialShaderTextureServicesCandidate&services)noexcept{
    try{
        if(uint32_t(kind)>2||!input.value.created)return false;
        auto next=input;const auto calls=services;const auto suppliedGroup=group;
        const auto type=next.value.value.numeric.typeAndMetadata&15u;
        if(type==3){if(!materialTypedTextureValidCandidate(next.value))return false;}
        else{
            MaterialTextureReferenceCandidate result;
            const auto ordinary=[](const MaterialTextureReferenceCandidate&r){return r.valid()&&!r.isEnvironment()&&bool(r.resource());};
            const auto error=[&](){if(!calls.errorTexture)return false;const auto r=calls.errorTexture();if(!r||!ordinary(*r))return false;result=*r;return true;};
            if(type!=1){if(!error())return false;}
            else{
                auto name=next.value.value.text;
                // Safe measured ASCII subset. Empty/native overlength and locale
                // behavior remain unavailable; this is not filesystem authorization.
                if(name.empty()||name.size()>259||name.find('\0')!=std::string::npos)return false;
                for(unsigned char c:name)if(c>=128)return false;
                if(suppliedGroup){if(suppliedGroup->size()>255||suppliedGroup->find('\0')!=std::string::npos)return false;
                    for(unsigned char c:*suppliedGroup)if(c>=128)return false;}
                const auto environment=[](std::string_view s){if(s.size()!=11)return false;constexpr std::string_view word="env_cubemap";
                    for(size_t i=0;i<s.size();++i){char c=s[i];if(c>='A'&&c<='Z')c=char(c+32);if(c!=word[i])return false;}return true;};
                bool isEnvironment=false;MaterialShaderTextureRequestCandidate request;request.kind=kind;request.flags=flags;request.group=suppliedGroup;
                if(kind==MaterialShaderTextureKindCandidate::Normal){
                    // Native condition tests either of the first two bytes, then
                    // advances exactly one byte, including the unusual a\\name case.
                    if(name[0]=='\\'||(name.size()>1&&name[1]=='\\'))name.erase(0,1);
                    if(!calls.configMask)return false;const auto config=calls.configMask();if(!config)return false;
                    isEnvironment=(*config&1u)!=0&&environment(name);
                }else if(kind==MaterialShaderTextureKindCandidate::Bump)request.flags=0;
                else{
                    isEnvironment=environment(name);request.group="CubeMap textures";
                    if(!isEnvironment){if(!calls.hdrType)return false;const auto hdr=calls.hdrType();if(!hdr)return false;
                        if(*hdr){const auto room=259-name.size();name.append(".hdr",std::min(size_t(4),room));}}
                }
                if(isEnvironment){result=MaterialTextureReferenceCandidate::environment();if(kind==MaterialShaderTextureKindCandidate::Cube)next.flags2|=32u;}
                else{
                    if(!calls.find)return false;request.name=name;const auto found=calls.find(request);if(!found)return false;
                    if(found->texture){if(!ordinary(*found->texture))return false;result=*found->texture;}
                    else{
                        if(kind==MaterialShaderTextureKindCandidate::Normal){
                            if(!calls.suppressWarning)return false;const auto suppress=calls.suppressWarning();if(!suppress)return false;
                            if(!(*suppress&255u)&&!environment(name)){if(!calls.warning)return false;calls.warning(name);}
                        }
                        if(!error())return false;
                    }
                }
            }
            if(!materialTypedTextureSetCandidate(next.value,result))return false;
        }
        static_assert(std::is_nothrow_move_assignable<MaterialShaderTextureStateCandidate>::value,"transaction publication must not throw");
        output=std::move(next);return true;
    }catch(...){return false;}
}
