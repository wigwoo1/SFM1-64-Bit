#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
// Original102505C0/10250680: ASCII case-insensitive substring search. Empty
// haystack OR empty needle returns null, unlike std::string_view::find("").
inline bool sourceCacheFilterContainsCandidate(std::string_view haystack,std::string_view needle){
    haystack=haystack.substr(0,haystack.find('\0'));needle=needle.substr(0,needle.find('\0'));
    if(haystack.empty()||needle.empty()||needle.size()>haystack.size())return false;
    auto lower=[](unsigned char c){return c>='A'&&c<='Z'?static_cast<unsigned char>(c+32):c;};
    for(size_t i=0;i<=haystack.size()-needle.size();++i){size_t j=0;while(j<needle.size()&&lower(static_cast<unsigned char>(haystack[i+j]))==lower(static_cast<unsigned char>(needle[j])))++j;if(j==needle.size())return true;}
    return false;
}
struct SourceModelCacheQuerySnapshotCandidate {
    std::optional<uintptr_t>cachedHandle;
    uintptr_t renderable=0;
    std::array<float,3>point{};
    int32_t diagnosticMode=0;
    bool instanceRecordPresent=false,neverAsString=false;
    std::optional<std::string_view>instanceName,filterString;
};
template<class LightingState>struct SourceModelCacheQueryResultCandidate {
    LightingState state{};
    uintptr_t value=0,auxiliary=0;
    std::optional<std::array<float,3>>positionScratch;
    bool filterCompared=false;
};
// Original100D5845..58BF /100D5A83..5B94. This routes acquisition; it does not
// implement a cache store, handle validity, record lifecycle or later blending.
// Services::LightingState is owned by the caller: no original32bit payload ABI
// is baked into this policy. Opaque identity/results retain native pointer width.
// A query may change the shared enabled byte, which must then be read again.
template<class Services>
SourceModelCacheQueryResultCandidate<typename Services::LightingState>sourceModelCacheQueryCandidate(
    const SourceModelCacheQuerySnapshotCandidate&s,Services&services,uintptr_t initialAuxiliary=0){
    SourceModelCacheQueryResultCandidate<typename Services::LightingState>out;out.auxiliary=initialAuxiliary;
    if(s.cachedHandle){
        if(services.enabled()){
            const uint32_t flags=services.valid(*s.cachedHandle)?7u:6u;
            out.state=services.cached(*s.cachedHandle,flags,out.value);
            if(services.enabled())return out;
        }
        out.state=services.cached(*s.cachedHandle,7u,out.value);return out;
    }
    for(float x:s.point)if(!std::isfinite(x))throw std::runtime_error("nonfinite selected cache query point");out.positionScratch=s.point;
    if(services.enabled()){
        out.value=services.point(s.point,out.state,out.auxiliary,s.renderable,6u,false);
        if(services.enabled())return out;
    }
    bool special=false;
    if(s.diagnosticMode==5&&s.instanceRecordPresent&&s.instanceName){
        out.filterCompared=true;const auto filter=s.neverAsString?std::string_view("FCVAR_NEVER_AS_STRING"):s.filterString.value_or(std::string_view{});
        special=sourceCacheFilterContainsCandidate(*s.instanceName,filter);
    }
    out.value=services.point(s.point,out.state,out.auxiliary,s.renderable,15u,special);return out;
}
