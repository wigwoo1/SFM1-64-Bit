#pragma once
#include "sfm_time_to_frame_candidate.h"
#include "sfm_frame_to_time_candidate.h"
// 1097b020: caller mode selects integer frame; output conversion always mode0.
inline std::int32_t sfmAdjustTimeCandidate(std::int32_t time,SfmFrameRateCandidate rate,unsigned mode) {
    const auto frame=sfmTimeToFrameCandidate(time,rate,mode);
    return sfmFrameToTimeCandidate(static_cast<float>(frame),rate,0,
        [](double value,unsigned rounding) {
            assert(rounding==0);
            return sfmDoubleTimeConvertCandidate(value,rounding,[](double){assert(false);return 0;});
        });
}
// 10298ad0: preserve object/attribute lookup and packed-rate construction order.
template<class Adapter>
inline std::int32_t editorAdjustTimeCandidate(std::int32_t time,unsigned mode,Adapter& a) {
    float rate=24.0f;
    if(auto* object=a.rateObject())
        if(auto* attribute=a.frameRateAttribute(object))rate=a.attributeFloat(attribute);
    const auto packed=a.constructRate(rate);
    return sfmAdjustTimeCandidate(time,packed,mode);
}
