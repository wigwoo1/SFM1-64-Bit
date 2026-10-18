#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
// Recovered fields are named by offset until their full semantic identities
// are established. Native storage keeps original 32-bit time representation.
struct EditorNormalTimeStateCandidate {
    std::int32_t field47c=0,modes480[4]{},time430=std::numeric_limits<std::int32_t>::min(),time684=0;
    bool flag69c=false,flag228=false,flag2a8=false,flag6c0=false;
};
template<class Adapter>
inline std::int32_t editorNormalTimeCandidate(const EditorNormalTimeStateCandidate& s,Adapter& a) {
    if(a.engineReady()) {
        bool active=s.field47c!=0;
        for(auto mode:s.modes480)if(mode==2)active=true;
        if(active) {
            const auto offset=s.time430;
            if(offset==std::numeric_limits<std::int32_t>::min())return s.time684;
            const auto bits=static_cast<std::uint32_t>(a.clockTime())-static_cast<std::uint32_t>(offset);
            std::int32_t result;std::memcpy(&result,&bits,sizeof(result));return result;
        }
    }
    const auto stored=s.time684;
    if(!s.flag69c && !s.flag228 && !s.flag2a8 && s.flag6c0)return a.adjustTime(stored,0);
    return stored;
}
