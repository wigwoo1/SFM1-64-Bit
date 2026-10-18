#pragma once
#include <cstdint>
// 1029aa20 branch and call order; native object/attribute ABI supplied by adapter.
template<class Adapter>
inline std::int32_t editorCurrentTimeCandidate(Adapter& a) {
    if(a.frameMode()) {
        float rate=24.0f;
        if(auto* object=a.rateObject())
            if(auto* attribute=a.frameRateAttribute(object))rate=a.attributeFloat(attribute);
        const auto packed=a.constructRate(rate);
        const auto frameFloat=static_cast<float>(a.frameIndex());
        return a.convertFrame(frameFloat,packed,0);
    }
    if(a.overrideEnabled())return a.overrideTime();
    return a.normalTime();
}
