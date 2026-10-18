#pragma once
#include <cstring>
#include <cstddef>
#include <cassert>
// 10975a20 normal length/positive destination capacity contract.
// Original null/empty source writes destination[0] even for zero capacity;
// zero/negative capacities and source/destination overlap are not supported here.
inline void extractMapBaseNameCandidate(const char* source,char* destination,std::size_t capacity) {
    assert(destination && capacity>0);
    if(!source || !*source) {destination[0]=0;return;}
    const auto last=static_cast<std::ptrdiff_t>(std::strlen(source))-1;
    auto end=last;
    while(end>0 && source[end]!='.' && source[end]!='/' && source[end]!='\\')--end;
    if(source[end]=='.')--end;else end=last;
    auto begin=last;
    while(begin>=0 && source[begin]!='/' && source[begin]!='\\')--begin;
    begin=begin>=0?begin+1:0;
    const auto wanted=static_cast<std::size_t>(end-begin+2);
    const auto count=wanted<capacity?wanted:capacity;
    // Computed count never exceeds remaining source bytes including its NUL.
    // Thus this path needs no strncpy padding; overlap remains unsupported.
    std::memcpy(destination,source+begin,count);
    if(count>0)destination[count-1]=0;
}
