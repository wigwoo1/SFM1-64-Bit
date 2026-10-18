#include "model_title_string_handle_candidate.h"
#include <cassert>
#include <iostream>
struct Payload{std::uintptr_t symbol;};struct Slot{std::uint32_t word;Payload* payload;};
struct Adapter{char name[6]="title";Payload p{reinterpret_cast<std::uintptr_t>(name)};Slot s{7,&p};unsigned slotCount(){return 1;}Slot& slot(unsigned){return s;}auto payloadSymbol(Payload* x){return x->symbol;}const char* emptyString(){return "";}};
int main(){Adapter a;const auto h=7u<<20;assert(modelTitleStringHandleCandidate(h,a)==a.name);
    assert(!modelTitleStringHandleCandidate(0xffffffff,a));assert(!modelTitleStringHandleCandidate(h|1,a));assert(!modelTitleStringHandleCandidate(6u<<20,a));
    a.s.word=0x80000007;assert(!modelTitleStringHandleCandidate(h,a));a.s.word=0x1007;assert(!modelTitleStringHandleCandidate(h,a));
    a.s.word=7;a.s.payload=nullptr;assert(!modelTitleStringHandleCandidate(h,a));a.s.payload=&a.p;
    a.p.symbol=std::numeric_limits<std::uintptr_t>::max();assert(modelTitleStringHandleCandidate(h,a) && *modelTitleStringHandleCandidate(h,a)=='\0');
    std::cout<<"x64 model title lookup: handle bounds/generation/inactive/null payload/native symbol/empty distinction passed\n";
}
