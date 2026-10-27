#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>

struct Graph;
class SessionModelProxyContextCandidate;
inline SessionModelProxyContextCandidate readSessionModelProxyContextCandidate(const Graph&,uint32_t);

// Graph-independent, value-owned identity. Only the checked session adapter
// constructs known provenance. A default value means unknown, not null entity.
class SessionModelProxyContextCandidate {
    uint32_t model_=UINT32_MAX;
    std::array<char,16>guid_{};
    std::string asset_;
    bool ordinaryPuppet_=false;
    friend SessionModelProxyContextCandidate readSessionModelProxyContextCandidate(const Graph&,uint32_t);
public:
    SessionModelProxyContextCandidate()=default;
    bool knownOrdinaryIfmPuppet()const noexcept{return ordinaryPuppet_;}
    std::optional<bool>gameEntityPresent()const noexcept{if(!ordinaryPuppet_)return {};return false;}
    uint32_t model()const noexcept{return model_;}
    const std::array<char,16>&guid()const noexcept{return guid_;}
    const std::string&asset()const noexcept{return asset_;}
    bool matches(uint32_t model,const std::array<char,16>&guid,const std::string&asset)const noexcept{
        return ordinaryPuppet_&&model_==model&&guid_==guid&&asset_==asset;
    }
};

// A per-draw binding, distinct from a bare source snapshot. The target identity
// must match at construction. Store frames own a copy, never a caller pointer.
class MaterialModelProxyBindContextCandidate {
    SessionModelProxyContextCandidate source_;
public:
    MaterialModelProxyBindContextCandidate()=default;
    static std::optional<MaterialModelProxyBindContextCandidate>checked(
            const SessionModelProxyContextCandidate&source,uint32_t model,
            const std::array<char,16>&guid,const std::string&asset){
        if(!source.matches(model,guid,asset))return {};
        MaterialModelProxyBindContextCandidate out;out.source_=source;return out;
    }
    bool verifiedPuppetWithoutEntity()const noexcept{
        return source_.knownOrdinaryIfmPuppet()&&source_.gameEntityPresent()==std::optional<bool>(false);
    }
    const SessionModelProxyContextCandidate&source()const noexcept{return source_;}
};
