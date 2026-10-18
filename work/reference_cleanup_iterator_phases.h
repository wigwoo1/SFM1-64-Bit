#pragma once
#include "reference_registration_state.h"
#include "reference_runtime_element.h"
//1035BAEF..1035BC70; each phase re-enumerates the caller's live-handle iterator.
template<class Enumerate,class Resolve,class Registration,class Mark,class Register,class Update,class Notify,class Remove>
void recoveredReferenceCleanupIteratorPhasesCandidate(Enumerate enumerate,Resolve resolve,Registration registration,Mark mark,Register registerReferences,Update update,Notify notify,Remove remove){
 enumerate([&](uint32_t h){auto*e=resolve(h);if(e&&e->cleanupContext!=0xffffffffu&&(!(e->record.flags&1)||(e->record.counts&0xffc00u)))mark(*e,1,0,0);});
 enumerate([&](uint32_t h){auto*e=resolve(h);if(!e||e->cleanupContext==0xffffffffu)return;auto&s=registration(*e);if(s.eligible(0)){if(s.suppressed())registerReferences(*e,0,false);}else if(s.eligible(1)&&!s.suppressed())registerReferences(*e,0,true);});
 enumerate([&](uint32_t h){auto*e=resolve(h);if(!e||e->cleanupContext==0xffffffffu)return;auto&s=registration(*e);if(s.eligible(0)){if(s.suppressed()){update(*e,false);notify(*e,false);}}else if(s.eligible(1)){if(!s.suppressed()){update(*e,true);notify(*e,true);}}else remove(h,0);});
}
