#pragma once
#include "recovered_frame_ticks_candidate.h"
#include <cstring>
// Ifm1097B020 mode0: sign-dependent integer adjustment, then float frame -> ticks.
// Rate denominator already contains the 10000 ticks/second scale.
inline int32_t recoveredTimeSnapCandidate(int32_t ticks,uint32_t packedRate){
 const uint32_t n=packedRate&65535u,d=packedRate>>16;
 if(!n||!d)throw std::runtime_error("invalid snap rate");
 int64_t product=int64_t(ticks)*n;
 product+=ticks<0?-int64_t(d/2):int64_t(d/2);
 // Original negative branch1097AF79 ->1097AFFB adds numerator before division.
 if(ticks<0)product+=n;
 const uint32_t frameWord=uint32_t(product/int64_t(d));
 int32_t frame;std::memcpy(&frame,&frameWord,4);
 return recoveredFrameTicksCandidate(float(frame),packedRate,0);
}
