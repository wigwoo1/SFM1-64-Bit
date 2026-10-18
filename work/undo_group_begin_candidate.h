#pragma once
#include <cstdint>
template<class Adapter>
void beginUndoGroupCandidate(const char* first,const char* second,std::uintptr_t context,Adapter& a) {
    if(!(a.flags74()&8))return;
    a.logGroupBegin(first);
    const std::uint32_t previous=a.depth54();
    a.setDepth54(previous+std::uint32_t{1});
    if(previous!=0)return;
    a.setPreviousContext(a.currentContext());a.setCurrentContext(context);
    const auto symbol=a.internLabel(first);a.setFirstLabel(symbol);
    a.setSecondLabel(first==second?symbol:a.internLabel(second));
    a.setFlags74(static_cast<std::uint8_t>(a.flags74()|1));
    a.setSubmissionCounter(0);
}
