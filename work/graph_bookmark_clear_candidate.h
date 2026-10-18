#pragma once
#include "graph_bookmark_alias_assignment_candidate.h"
template<class Adapter>void graphBookmarkClearCandidate(Adapter& a){
 if(!static_cast<unsigned char>(a.mutationGate103623b0(a.attribute4())))return;
 if(static_cast<unsigned char>(a.recordModelSlot62(a.owner8(a.attribute4())))){
  auto* undo=a.allocateNativeUndo();
  if(undo)undo=a.construct1036c020(undo,a.attribute4(),nullptr,0,0);
  a.submitModelSlot79(undo);
 }
 auto* attribute=a.attribute4();const auto flags=a.flags11(attribute);
 const auto count=a.count0c(a.vector8());auto* owner=a.owner8(attribute);
 if(flags&1)if(static_cast<unsigned char>(a.ownerReady1035dc00(owner)))
  a.ownerSlot2(owner,a.attribute4(),0,graphAliasDifference(count,1));
 a.setCount0c(a.vector8(),0);
 attribute=a.attribute4();
 if(a.flags11(attribute)&1)if(static_cast<unsigned char>(a.ownerReady1035dc00(a.owner8(attribute))))
  a.ownerSlot0(a.owner8(attribute),attribute);
 const auto type=a.type10(attribute)&31;const bool references=type==1||type==15;
 a.markOwner1035eff0(a.owner8(attribute),references);
 a.orFrameworkFlags(references?0x1000u:0x4000u);
}
