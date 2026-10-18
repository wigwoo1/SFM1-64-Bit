#pragma once
#include <cstdint>
#include <cstring>
template<class Clip,class Adapter>
inline std::int32_t clipStartTimeCandidate(Clip* clip,Adapter& a) {
    if(auto* frame=a.resolveFrame(a.frameHandle(clip)))return a.frameStart(frame);
    return 0;
}
template<class Clip,class Adapter>
inline std::int32_t clipEndTimeCandidate(Clip* clip,Adapter& a) {
    if(auto* frame=a.resolveFrame(a.frameHandle(clip))) {
        // x86 ADD wraps modulo2^32; avoid C++ signed-overflow undefined behavior.
        const auto bits=static_cast<std::uint32_t>(a.frameStart(frame))+
            static_cast<std::uint32_t>(a.frameDuration(frame));
        std::int32_t value;std::memcpy(&value,&bits,sizeof(value));return value;
    }
    return 0;
}
