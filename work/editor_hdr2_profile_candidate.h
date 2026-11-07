#pragma once
#include "scene_hdr2_profile_candidate.h"
#include <fstream>
#include <locale>
#include <iostream>
#include <sstream>

inline std::shared_ptr<const SceneHdr2ProfileCandidate> editorHdr2ProfileCandidate;
inline std::string editorHdr2SummaryCandidate(const SceneHdr2PreparedFrameCandidate& frame){
    size_t fallback=0;for(const auto* list:{&frame.plan()->opaque,&frame.plan()->translucent})for(const auto& batch:*list)fallback+=batch.faceResource==SIZE_MAX;
    return "Experimental HDR2: "+std::to_string(frame.plan()->faces.size())+" lit faces, "+std::to_string(fallback)+" fallback batches. "+(frame.profile()->usesSessionExposure()?"Session/camera exposure on admitted lightmaps only. ":"")+"Source appearance unverified.";
}
// Explicit experimental startup policy, never stored in the session or inferred
// from native CVars. All fields are mandatory in the documented fixed order.
inline std::shared_ptr<const SceneHdr2ProfileCandidate> readEditorHdr2ProfileCandidate(std::istream& input){
    input.imbue(std::locale::classic());
    auto word=[&](const char* expected){std::string value;if(!(input>>value)||value!=expected)throw std::runtime_error("HDR2 profile expected "+std::string(expected));};
    SourceHdr2ScenePolicyCandidate p{};uint32_t config=0,padding=0,initial=0;std::string filter;
    std::string version;input>>version;
    if(version=="SFM64_HDR2_PROFILE_DIRECTIONAL_TEST_V1")p.directionalWorld=true;
    else if(version!="SFM64_HDR2_PROFILE_V1")throw std::runtime_error("unknown HDR2 profile version");
    word("selectedHdrType");input>>p.selectedHdrType;
    word("encoding");input>>p.encoding;word("styleScale");input>>p.styleScale;
    word("instance");for(auto& value:p.instance)input>>value;
    word("configurationByte49");input>>config;word("exposure");
    std::string exposure;input>>exposure;const bool sessionExposure=exposure=="session_camera";
    if(sessionExposure)p.exposure=1;
    else{std::istringstream number(exposure);number.imbue(std::locale::classic());number>>p.exposure;char extra;if(!number||(number>>extra))throw std::runtime_error("HDR2 exposure value refused");}
    word("pagePadding");input>>padding;word("pageInitialByte");input>>initial;
    word("filter");input>>filter;
    if(!input||config>255||initial>255||(filter!="nearest"&&filter!="linear"))throw std::runtime_error("HDR2 profile value refused");
    std::string extra;if(input>>extra)throw std::runtime_error("HDR2 profile trailing field");
    return std::make_shared<const SceneHdr2ProfileCandidate>(p,padding,uint8_t(initial),filter=="linear"?SceneHdr2FilterCandidate::Linear:SceneHdr2FilterCandidate::Nearest,sessionExposure);
}
inline void configureEditorHdr2ProfileCandidate(){
    char path[32768]{};auto n=GetEnvironmentVariableA("SFM64_EXPERIMENTAL_HDR2_PROFILE",path,sizeof(path));
    if(!n)return;
    if(n>=sizeof(path)||!GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK",nullptr,0))throw std::runtime_error("HDR2 profile requires a retained session and valid profile path");
    for(const char* option:{"SFM64_DIAGNOSTIC_SELFILLUM_TEST","SFM64_DIAGNOSTIC_NORMAL_VIEW_TEST","SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST","SFM64_DIAGNOSTIC_EYE_CANDIDATE","SFM64_DIAGNOSTIC_LIGHT_DRAW_TEST","SFM64_DIAGNOSTIC_OUTPUT_SRGB_TEST","SFM64_DIAGNOSTIC_MATERIAL_COLOR_TEST","SFM64_DIAGNOSTIC_BASE_SRGB_TEST"})
        if(GetEnvironmentVariableA(option,nullptr,0))throw std::runtime_error(std::string("HDR2 explicit profile conflicts with ")+option);
    std::ifstream input(path);if(!input)throw std::runtime_error("HDR2 profile file unavailable");
    auto next=readEditorHdr2ProfileCandidate(input);editorHdr2ProfileCandidate=std::move(next);
    std::cout<<"experimental_hdr2_profile=1 scene=RGBA16_SFLOAT output=encoded_UNORM base_read=material_metadata source_parity=unverified\n";
}
