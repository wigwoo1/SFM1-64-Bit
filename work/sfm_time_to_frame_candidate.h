#pragma once
#include "sfm_frame_rate_candidate.h"
#include <cassert>
#include <cstring>
// 1097af20. Nonzero units required: original helper faults on division by zero.
inline std::int32_t sfmTimeToFrameCandidate(std::int32_t time,SfmFrameRateCandidate rate,unsigned mode) {
    assert(rate.timeUnits!=0);
    std::int64_t product=static_cast<std::int64_t>(time)*rate.numerator;
    if(time<0) {
        if(mode==0)product=product-rate.timeUnits/2+rate.numerator;
        else if(mode==1)product=product-rate.timeUnits+rate.numerator;
    } else {
        if(mode==0)product+=rate.timeUnits/2;
        else if(mode==2)product=product-rate.numerator+rate.timeUnits;
        else if(mode==1 || mode==3)product+=rate.numerator;
    }
    const auto bits=static_cast<std::uint32_t>(product/rate.timeUnits);
    std::int32_t result;std::memcpy(&result,&bits,sizeof(result));return result;
}
