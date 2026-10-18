#include "model_undo_record_gate_candidate.h"
#include "notification_suppression_flag_candidate.h"
#include <cassert>
#include <iostream>
struct Owner{std::uint32_t field38=42;};
struct Adapter{std::uint8_t flags=0;unsigned queries=0;bool undoRecordEnabled(){++queries;return modelUndoRecordEnabledCandidate(flags);}auto ownerField38(Owner* p){return p->field38;}};
int main(){
    for(unsigned flags=0;flags<256;++flags)for(unsigned input=0;input<256;++input){
        auto value=static_cast<std::uint8_t>(flags);setModelUndoRecordEnabledCandidate(value,static_cast<std::uint8_t>(input));
        assert(modelUndoRecordEnabledCandidate(value)==(input!=0));assert((static_cast<unsigned>(value)&0xf7u)==(flags&0xf7u));
        const bool enabled=modelUndoRecordEnabledCandidate(value);setNotificationSuppressionFlagCandidate(value,static_cast<std::uint8_t>(input));assert(modelUndoRecordEnabledCandidate(value)==enabled);
    }
    Owner o;Adapter a;a.flags=8;assert(modelAllowsUndoRecordCandidate(&o,a));
    o.field38=0xffffffff;assert(!modelAllowsUndoRecordCandidate(&o,a));
    const auto before=a.queries;assert(!modelAllowsUndoRecordCandidate(static_cast<Owner*>(nullptr),a) && a.queries==before+1);
    a.flags=0;o.field38=42;assert(!modelAllowsUndoRecordCandidate(&o,a));
    std::cout<<"x64 undo enable: 65536 flag/input cases/shared suppression independence/owner gate passed\n";
}

