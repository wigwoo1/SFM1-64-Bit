#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
// Explicit measured service/data snapshot for a numerical prerequisite.
// Not a live entity adapter, material proxy registry entry or engine ABI.
// flags: result1, renderable2, resolved entity4, owner8, owner-player16,
// viewer32, viewer-player64, observer-target-is-owner128.
// Remaining words: initial material type, packed owner handle, table serial,
// owner/viewer selectors, observer mode, raw cloak, limit, local slot, old cache.
inline std::optional<uint32_t>materialWeaponInvisValueCandidate(const std::array<uint32_t,11>&q){
    const auto flags=q[0];if((flags&7u)!=7u)return {};
    if(q[2]==0xffffffffu||(q[2]>>16)!=q[3]||!(flags&8u)||!(flags&16u))return 0u;
    auto bits=q[7];if((bits&0x7fffffffu)>0x7f800000u)bits|=0x400000u; // x87 return/store
    const bool viewer=(flags&32u)&&(flags&64u);
    const bool opposite=viewer&&((q[5]==2u&&q[4]==3u)||(q[5]==3u&&q[4]==2u));
    const bool observed=viewer&&(q[6]==1u||q[6]==2u)&&(flags&128u);
    if(!opposite||observed){float value,limit;std::memcpy(&value,&bits,4);std::memcpy(&limit,&q[8],4);if(value>limit)bits=q[8];}
    return bits;
}
