#pragma once
#include <cstdint>
template<class Adapter>int graphBookmarkAttributeInsertCandidate(int index,const std::uint32_t* value,Adapter& a){
 if(!static_cast<unsigned char>(a.mutationGate103623b0(a.attribute4())))return -1;
 if(static_cast<unsigned char>(a.recordModelSlot62(a.owner8(a.attribute4())))){
  auto* undo=a.allocateNativeInsertUndo();
  if(undo){a.constructInsertIdentity10364bd0(undo,a.attribute4());a.setInsertTable(undo,0x10b386dc);a.setInsertIndex1c(undo,index);a.setInsertValue20(undo,*value);}
  a.submitModelSlot79(undo);
 }
 const auto result=a.insert1008f550(a.vector8(),index,value);
 auto* attribute=a.attribute4();const auto flags=a.flags11(attribute);auto* owner=a.owner8(attribute);
 if(flags&1)if(static_cast<unsigned char>(a.ownerReady1035dc00(owner)))
  a.ownerSlot1(owner,a.attribute4(),result,result);
 attribute=a.attribute4();
 if(a.flags11(attribute)&1)if(static_cast<unsigned char>(a.ownerReady1035dc00(a.owner8(attribute))))
  a.ownerSlot0(a.owner8(attribute),attribute);
 const auto type=a.type10(attribute)&31;const bool references=type==1||type==15;
 a.markOwner1035eff0(a.owner8(attribute),references);a.orFrameworkFlags(references?0x1000u:0x4000u);
 return result;
}
