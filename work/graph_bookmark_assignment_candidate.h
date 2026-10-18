#pragma once
#include "graph_bookmark_alias_assignment_candidate.h"
template<class Source,class Adapter>void graphBookmarkAssignmentCandidate(Source* source,int count,Adapter& a){
 if(graphBookmarkAliasAssignmentCandidate(source,count,a))return;
 if(!static_cast<unsigned char>(a.mutationGate103623b0()))return;
 if(static_cast<unsigned char>(a.recordOwnerModelSlot62())){
  auto* undo=a.allocateNativeUndo();if(undo)undo=a.construct1036c020(undo,source,count,0);
  a.submitModelSlot79(undo);
 }
 {
  auto* owner=a.owner8();const auto old=a.destinationCount();
  if(a.flags11()&1)if(static_cast<unsigned char>(a.ownerReady1035dc00(owner)))a.ownerSlot2(owner,0,graphAliasDifference(old,1));
 }
 a.copy1038a010(source,count);
 {
  const auto size=a.destinationCount();auto* owner=a.owner8();
  if(a.flags11()&1)if(static_cast<unsigned char>(a.ownerReady1035dc00(owner)))a.ownerSlot1(owner,0,graphAliasDifference(size,1));
 }
 if(a.flags11()&1)if(static_cast<unsigned char>(a.ownerReady1035dc00(a.owner8())))a.ownerSlot0(a.owner8());
 const auto type=a.type10()&31;const bool references=type==1 || type==15;
 a.mark1035eff0(references);a.orFrameworkFlags(references?0x1000u:0x4000u);
}
