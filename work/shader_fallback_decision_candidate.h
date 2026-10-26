#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
// Per-shader GetFallback decision only, after parameter initialization in the
// real material caller. Never follow this table as a substitute for recollecting
// and initializing parameters for each selected shader or fallback VMT.
enum class ShaderFallbackKindCandidate{Literal,Level90,Level92,Capability95,MonitorBase,ProtoIce,Prototype,Level95,WaterHDR};
struct ShaderFallbackEntryCandidate{std::string_view name;ShaderFallbackKindCandidate kind;const char*literal;};
inline const std::array<ShaderFallbackEntryCandidate,157>&shaderFallbackEntriesCandidate(){
    static constexpr std::array<ShaderFallbackEntryCandidate,157>entries{{
        {"accumbuff4sample",ShaderFallbackKindCandidate::Literal,nullptr},
        {"accumbuff5sample",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Aftershock",ShaderFallbackKindCandidate::Literal,"Aftershock_dx9"},
        {"Aftershock_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Bik",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Blob",ShaderFallbackKindCandidate::Literal,"Blob_dx9"},
        {"Blob_dx9",ShaderFallbackKindCandidate::Level90,nullptr},
        {"Bloom",ShaderFallbackKindCandidate::Literal,nullptr},
        {"BlurFilterX",ShaderFallbackKindCandidate::Literal,nullptr},
        {"BlurFilterY",ShaderFallbackKindCandidate::Literal,nullptr},
        {"BufferClearObeyStencil",ShaderFallbackKindCandidate::Literal,"BufferClearObeyStencil_DX9"},
        {"BufferClearObeyStencil_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Cable",ShaderFallbackKindCandidate::Literal,"Cable_DX9"},
        {"Cable_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Cloak",ShaderFallbackKindCandidate::Literal,"Cloak_DX90"},
        {"Cloak_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"ColorCorrection",ShaderFallbackKindCandidate::Literal,nullptr},
        {"CustomHero",ShaderFallbackKindCandidate::Literal,"CustomHero_dx9"},
        {"CustomHero_dx9",ShaderFallbackKindCandidate::Level90,nullptr},
        {"DebugMorphAccumulator",ShaderFallbackKindCandidate::Literal,nullptr},
        {"DebugMRTTexture",ShaderFallbackKindCandidate::Literal,nullptr},
        {"DebugTextureView",ShaderFallbackKindCandidate::Literal,"DebugTextureView_dx9"},
        {"DebugTextureView_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"DecalModulate",ShaderFallbackKindCandidate::Literal,"DecalModulate_DX9"},
        {"DecalModulate_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"deferred_global_light",ShaderFallbackKindCandidate::Literal,nullptr},
        {"deferred_post_process",ShaderFallbackKindCandidate::Literal,nullptr},
        {"deferred_simple_light",ShaderFallbackKindCandidate::Literal,nullptr},
        {"deferred_specular_bloom",ShaderFallbackKindCandidate::Literal,nullptr},
        {"deferred_unlit",ShaderFallbackKindCandidate::Literal,nullptr},
        {"DepthOfField",ShaderFallbackKindCandidate::Literal,"DepthOfField_dx9"},
        {"DepthOfField_dx9",ShaderFallbackKindCandidate::Level92,nullptr},
        {"DepthWrite",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Downsample",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Downsample_nohdr",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Engine_Post",ShaderFallbackKindCandidate::Literal,"Engine_Post_dx9"},
        {"Engine_Post_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"EyeRefract",ShaderFallbackKindCandidate::Literal,"EyeRefract_dx9"},
        {"EyeRefract_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"EyeGlint",ShaderFallbackKindCandidate::Literal,"EyeGlint_dx9"},
        {"EyeGlint_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"eyes",ShaderFallbackKindCandidate::Literal,"Eyes_dx9"},
        {"Eyes_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"FilmDust",ShaderFallbackKindCandidate::Literal,nullptr},
        {"FilmGrain",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Flesh",ShaderFallbackKindCandidate::Literal,"Flesh_dx9"},
        {"Flesh_dx9",ShaderFallbackKindCandidate::Level90,nullptr},
        {"floatcombine",ShaderFallbackKindCandidate::Literal,nullptr},
        {"floatcombine_autoexpose",ShaderFallbackKindCandidate::Literal,nullptr},
        {"floattoscreen",ShaderFallbackKindCandidate::Literal,nullptr},
        {"floattoscreen_vanilla",ShaderFallbackKindCandidate::Literal,nullptr},
        {"GAMECONTROLS",ShaderFallbackKindCandidate::Literal,nullptr},
        {"GlobalLitSimple",ShaderFallbackKindCandidate::Literal,"GlobalLitSimple_DX90"},
        {"GlobalLitSimple_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"HDRCombineTo16Bit",ShaderFallbackKindCandidate::Literal,nullptr},
        {"HDRSelectRange",ShaderFallbackKindCandidate::Literal,nullptr},
        {"hsl_filmgrain_pass1",ShaderFallbackKindCandidate::Literal,nullptr},
        {"hsl_filmgrain_pass2",ShaderFallbackKindCandidate::Literal,nullptr},
        {"HSV",ShaderFallbackKindCandidate::Literal,nullptr},
        {"IceSurface",ShaderFallbackKindCandidate::Literal,"IceSurface_dx9"},
        {"IceSurface_dx9",ShaderFallbackKindCandidate::Level90,nullptr},
        {"Infected",ShaderFallbackKindCandidate::Literal,nullptr},
        {"IntroScreenSpaceEffect",ShaderFallbackKindCandidate::Literal,nullptr},
        {"LightmappedGeneric",ShaderFallbackKindCandidate::Literal,nullptr},
        {"LightmappedReflective",ShaderFallbackKindCandidate::Literal,"LightmappedReflective_DX90"},
        {"LightmappedReflective_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"LightShafts",ShaderFallbackKindCandidate::Literal,"LightShafts_dx9"},
        {"LightShafts_dx9",ShaderFallbackKindCandidate::Capability95,nullptr},
        {"ManifoldSurface",ShaderFallbackKindCandidate::Literal,"ManifoldSurface_dx9"},
        {"ManifoldSurface_dx9",ShaderFallbackKindCandidate::Level90,nullptr},
        {"Modulate",ShaderFallbackKindCandidate::Literal,"Modulate_DX9"},
        {"Modulate_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"MonitorScreen",ShaderFallbackKindCandidate::Literal,"MonitorScreen_DX9"},
        {"MonitorScreen_DX9",ShaderFallbackKindCandidate::MonitorBase,nullptr},
        {"MorphAccumulate",ShaderFallbackKindCandidate::Literal,"MorphAccumulate_DX9"},
        {"MorphAccumulate_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"MorphWeight",ShaderFallbackKindCandidate::Literal,"MorphWeight_DX9"},
        {"MorphWeight_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"MotionBlur",ShaderFallbackKindCandidate::Literal,"MotionBlur_dx9"},
        {"MotionBlur_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Multiblend",ShaderFallbackKindCandidate::Literal,"Multiblend_DX90"},
        {"Multiblend_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"ObjectMotionBlur",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Occlusion",ShaderFallbackKindCandidate::Literal,"Occlusion_DX9"},
        {"Occlusion_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"paintblob",ShaderFallbackKindCandidate::Literal,"paintblob_dx9"},
        {"paintblob_dx9",ShaderFallbackKindCandidate::Level90,nullptr},
        {"ParallaxTest",ShaderFallbackKindCandidate::Literal,nullptr},
        {"ParticleLitGeneric_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"ParticleSphere",ShaderFallbackKindCandidate::Literal,"ParticleSphere_DX9"},
        {"ParticleSphere_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Portal",ShaderFallbackKindCandidate::Literal,"Portal_DX90"},
        {"Portal_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"PortalRefract",ShaderFallbackKindCandidate::Literal,"PortalRefract_dx9"},
        {"PortalRefract_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"PortalStaticOverlay",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Projected",ShaderFallbackKindCandidate::Literal,"Projected_DX90"},
        {"Projected_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"ProtoIce",ShaderFallbackKindCandidate::Literal,"ProtoIce_dx9"},
        {"ProtoIce_dx9",ShaderFallbackKindCandidate::ProtoIce,nullptr},
        {"Prototype",ShaderFallbackKindCandidate::Literal,"Prototype_dx9"},
        {"Prototype_dx9",ShaderFallbackKindCandidate::Prototype,nullptr},
        {"Refract",ShaderFallbackKindCandidate::Literal,"Refract_DX90"},
        {"Refract_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Sample4x4",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Sample4x4_Blend",ShaderFallbackKindCandidate::Literal,nullptr},
        {"screenspace_general",ShaderFallbackKindCandidate::Literal,"screenspace_general_dx9"},
        {"screenspace_general_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"sfm_ambientocclusion_shader",ShaderFallbackKindCandidate::Literal,nullptr},
        {"sfm_ao_blur_shader",ShaderFallbackKindCandidate::Literal,nullptr},
        {"sfm_blurfilterx_shader",ShaderFallbackKindCandidate::Level95,nullptr},
        {"sfm_blurfiltery_shader",ShaderFallbackKindCandidate::Level95,nullptr},
        {"sfm_downsample_shader",ShaderFallbackKindCandidate::Literal,nullptr},
        {"sfm_integercombine_shader",ShaderFallbackKindCandidate::Literal,nullptr},
        {"sfm_shape_shader",ShaderFallbackKindCandidate::Level95,nullptr},
        {"Shadow",ShaderFallbackKindCandidate::Literal,nullptr},
        {"ShadowBuild",ShaderFallbackKindCandidate::Literal,"ShadowBuild_DX9"},
        {"ShadowBuild_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"ShadowModel",ShaderFallbackKindCandidate::Literal,"ShadowModel_DX9"},
        {"ShadowModel_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"ShatteredGlass",ShaderFallbackKindCandidate::Literal,nullptr},
        {"showz",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Sky",ShaderFallbackKindCandidate::Literal,"Sky_HDR_DX9"},
        {"Sky_HDR_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"SplineRope",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Sprite",ShaderFallbackKindCandidate::Literal,"Sprite_DX9"},
        {"Sprite_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Spritecard",ShaderFallbackKindCandidate::Literal,nullptr},
        {"SSAO",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Teeth",ShaderFallbackKindCandidate::Literal,"Teeth_DX9"},
        {"Teeth_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"TreeLeaf",ShaderFallbackKindCandidate::Literal,nullptr},
        {"UI",ShaderFallbackKindCandidate::Literal,"UI_DX90"},
        {"UI_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"UnlitGeneric",ShaderFallbackKindCandidate::Literal,nullptr},
        {"UnlitTwoTexture",ShaderFallbackKindCandidate::Literal,"UnlitTwoTexture_DX9"},
        {"UnlitTwoTexture_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"VertexLit_SOB",ShaderFallbackKindCandidate::Literal,nullptr},
        {"VertexLitGeneric",ShaderFallbackKindCandidate::Literal,nullptr},
        {"VolumeClouds",ShaderFallbackKindCandidate::Literal,"VolumeClouds_dx9"},
        {"VolumeClouds_dx9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"VortWarp",ShaderFallbackKindCandidate::Literal,"VortWarp_dx9"},
        {"VortWarp_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Water",ShaderFallbackKindCandidate::Literal,"Water_DX9_HDR"},
        {"Water_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Water_DX9_HDR",ShaderFallbackKindCandidate::WaterHDR,nullptr},
        {"WindowImposter",ShaderFallbackKindCandidate::Literal,"WindowImposter_DX90"},
        {"WindowImposter_DX90",ShaderFallbackKindCandidate::Literal,nullptr},
        {"Wireframe",ShaderFallbackKindCandidate::Literal,"Wireframe_DX9"},
        {"Wireframe_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"WorldTwoTextureBlend",ShaderFallbackKindCandidate::Literal,nullptr},
        {"WorldVertexTransition",ShaderFallbackKindCandidate::Literal,"WorldVertexTransition_DX9"},
        {"WorldVertexTransition_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"WriteStencil",ShaderFallbackKindCandidate::Literal,"WriteStencil_DX9"},
        {"WriteStencil_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
        {"WriteZ",ShaderFallbackKindCandidate::Literal,"WriteZ_DX9"},
        {"WriteZ_DX9",ShaderFallbackKindCandidate::Literal,nullptr},
    }};return entries;
}
struct ShaderFallbackEnvironmentCandidate{
    std::array<int32_t,2>level17{92,92};
    int32_t capability24=95;
    // Raw complete return word: original Water checks all32 bits, not just AL.
    uint32_t return34=0;
    bool configPresent=true,parametersPresent=true;
    uint8_t configByte38=0,baseTypeByte=6;
};
struct ShaderFallbackDecisionCandidate{
    std::optional<std::string>fallback;
    std::vector<uint32_t>hardwareQueries;
};
inline bool shaderFallbackDecisionCandidate(ShaderFallbackDecisionCandidate&out,size_t index,
        const ShaderFallbackEnvironmentCandidate&env){
    const auto&entries=shaderFallbackEntriesCandidate();if(index>=entries.size())return false;
    ShaderFallbackDecisionCandidate result;const auto&e=entries[index];size_t levelQueries=0;
    auto level=[&](){result.hardwareQueries.push_back(17);return env.level17[levelQueries++];};
    auto wire=[&](){result.fallback="Wireframe";};
    switch(e.kind){
    case ShaderFallbackKindCandidate::Literal:if(e.literal)result.fallback=e.literal;break;
    case ShaderFallbackKindCandidate::Level90:if(level()<90)wire();break;
    case ShaderFallbackKindCandidate::Level92:if(level()<92)wire();break;
    case ShaderFallbackKindCandidate::Level95:if(level()<95)wire();break;
    case ShaderFallbackKindCandidate::Capability95:
        result.hardwareQueries.push_back(24);if(env.capability24<95)wire();break;
    case ShaderFallbackKindCandidate::MonitorBase:
        if(env.parametersPresent&&(env.baseTypeByte&15)==6)result.fallback="LightmappedGeneric";break;
    case ShaderFallbackKindCandidate::ProtoIce:
        if(level()<90||level()<92)wire();
        else if(env.configPresent&&env.configByte38)result.fallback="VertexLitGeneric";break;
    case ShaderFallbackKindCandidate::Prototype:if(level()<90||level()<92)wire();break;
    case ShaderFallbackKindCandidate::WaterHDR:
        result.hardwareQueries.push_back(34);if(!env.return34)result.fallback="WATER_DX90";break;
    }
    out=std::move(result);return true;
}
