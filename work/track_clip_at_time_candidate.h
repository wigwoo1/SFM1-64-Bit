#pragma once
#include <cstdint>
// 104d3760: original signed integer time representation, not floating seconds.
template<class Track,class Adapter>
inline auto findTrackClipAtTimeCandidate(Track* track,std::int32_t time,Adapter& a)
    ->decltype(a.resolve(std::uint32_t{})) {
    using Pointer=decltype(a.resolve(std::uint32_t{}));
    if(a.trackType(track)!=3 || a.count(track)==0)return Pointer{};
    a.prepareTrack(track);
    const auto count=a.count(track);
    for(std::int32_t i=0;i<count;++i) {
        auto* clip=a.resolve(a.handles(track)[i]);
        if(!clip || !a.eligible(clip))continue;
        if(a.start(clip)>time)continue;
        if(a.end(clip)>time)return clip;
    }
    return Pointer{};
}
