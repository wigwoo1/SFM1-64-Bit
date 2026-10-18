#pragma once
#include "sfm_frame_rate_candidate.h"
inline std::int32_t sfmFloorDoubleTimeCandidate(double value) {
    auto x=_mm_set_sd(value),sign=_mm_and_pd(_mm_set_sd(-0.0),x);
    auto magnitude=_mm_xor_pd(x,sign),threshold=_mm_set_sd(4503599627370496.0);
    auto offset=_mm_or_pd(_mm_and_pd(threshold,_mm_cmplt_sd(magnitude,threshold)),sign);
    auto rounded=_mm_sub_sd(_mm_add_sd(x,offset),offset);
    auto correction=_mm_and_pd(_mm_cmpnle_sd(_mm_sub_sd(rounded,x),sign),_mm_set_sd(1.0));
    return _mm_cvttsd_si32(_mm_sub_sd(rounded,correction));
}
// 1097ab80 mode2 remains an adapter for CRT ceil/x87-to-int helpers.
template<class CeilConvert>
inline std::int32_t sfmDoubleTimeConvertCandidate(double value,unsigned mode,CeilConvert&& ceilConvert) {
    if(mode==0)return sfmFloorDoubleTimeCandidate(_mm_cvtsd_f64(_mm_add_sd(_mm_set_sd(value),_mm_set_sd(0.5))));
    if(mode==1)return sfmFloorDoubleTimeCandidate(value);
    if(mode==2)return ceilConvert(value);
    return _mm_cvttsd_si32(_mm_set_sd(value));
}
template<class Convert>
inline std::int32_t sfmFrameToTimeCandidate(float frame,SfmFrameRateCandidate rate,unsigned mode,Convert&& convert) {
    // Original receives float frame, widens to double, multiplies units first,
    // then divides by numerator. Keep division-by-zero behavior in the adapter.
    auto value=_mm_div_sd(_mm_mul_sd(_mm_set_sd(static_cast<double>(rate.timeUnits)),
        _mm_set_sd(static_cast<double>(frame))),_mm_set_sd(static_cast<double>(rate.numerator)));
    return convert(_mm_cvtsd_f64(value),mode);
}
