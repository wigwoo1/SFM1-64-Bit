#include "controller_child_title_source_candidate.h"
#include "model_title_string_handle_candidate.h"
#include "test_owner_symbol_composition_fixture.h"
#include <cassert>
#include <iostream>
struct Payload{std::uintptr_t symbol;};struct Slot{std::uint32_t word;Payload* payload;};
struct Owner{std::uint32_t field38;};
struct Adapter {
    OwnerSymbolCompositionFixture symbols;
    Payload payload{0};Slot entry{3,&payload};Owner owner{3u<<20};bool present=true;
    std::uint32_t childHandle80(){return 1;}Owner* resolveOwner(std::uint32_t){return present?&owner:nullptr;}
    auto ownerField38(Owner* p){return p->field38;}
    unsigned slotCount(){return 1;}Slot& slot(unsigned){return entry;}
    auto payloadSymbol(Payload* p){return p->symbol;}const char* emptyString(){return "";}
    const char* modelSlot97(std::uint32_t h){return modelTitleStringHandleCandidate(h,*this);}
};
int main(){Adapter a;a.payload.symbol=a.symbols.intern("sessions/example.dmx");
    const char* result=controllerChildTitleSourceCandidate(a);
    assert(result==reinterpret_cast<const char*>(a.payload.symbol));assert(std::string(result)=="sessions/example.dmx");
    assert(a.payload.symbol>0xffffffffull);
    a.entry.word=4;assert(!controllerChildTitleSourceCandidate(a));
    a.entry.word=3;a.payload.symbol=std::numeric_limits<std::uintptr_t>::max();assert(std::string(controllerChildTitleSourceCandidate(a)).empty());
    a.present=false;assert(!controllerChildTitleSourceCandidate(a));
    std::cout<<"x64 controller title composition: native interned pointer/handle lookup/stale generation/empty/missing passed\n";
}
