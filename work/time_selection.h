#pragma once
#include "time_scale.h"
#include <array>
#include <cstring>
inline int32_t wrapTicks(uint32_t n){int32_t v;std::memcpy(&v,&n,4);return v;}
inline int32_t timeToChild(int32_t tick,int32_t start,int32_t duration,int32_t offset,float scale,bool clamp){
 auto relative=wrapTicks(uint32_t(tick)-uint32_t(start));
 if(clamp){if(relative>duration)relative=duration;else if(relative<0)relative=0;}
 return scaleTicks(wrapTicks(uint32_t(relative)+uint32_t(offset)),scale);
}
// Four-boundary ToChildMediaTime overload recovered at ifm VA 0x104FA800.
inline std::array<int32_t,4> selectionToChild(std::array<int32_t,4> t,int32_t start,int32_t offset,float scale,bool fixCollapsed){
 auto transform=[&](int32_t n){return scaleTicks(wrapTicks(uint32_t(n)-uint32_t(start)+uint32_t(offset)),scale);};
 constexpr int32_t negativeSentinel=-2147483647,positiveSentinel=2147483647;
 if(t[0]==negativeSentinel||t[1]==negativeSentinel)t[0]=t[1]=negativeSentinel;
 else {t[0]=transform(t[0]);t[1]=transform(t[1]);if(fixCollapsed&&t[0]==t[1])t[0]=wrapTicks(uint32_t(t[0])-1);}
 if(t[2]==positiveSentinel||t[3]==positiveSentinel)t[2]=t[3]=positiveSentinel;
 else {t[3]=transform(t[3]);t[2]=transform(t[2]);if(fixCollapsed&&t[2]==t[3])t[2]=wrapTicks(uint32_t(t[2])-1);}
 return t;
}
