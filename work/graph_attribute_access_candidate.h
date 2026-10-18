#pragma once
#include "graph_typed_attribute_candidate.h"
template<class Element,class Adapter>auto graphFindAttributeCandidate(Element* owner,const char* name,Adapter& a){
 const auto symbol=a.internModelSlot50(name);auto* attribute=a.head30(owner);
 while(attribute){if(a.symbol14(attribute)==symbol)return attribute;attribute=a.next0(attribute);}return attribute;
}
template<class Attribute,class Adapter>std::uint32_t* graphAttributeHandleStorageCandidate(Attribute* attribute,GraphAttributeDefaultCandidate& state,std::uint32_t& scratch,Adapter& a){
 if(!(state.guard&1)){state.guard|=1;state.handle=0xffffffff;state.field14=0xffffffff;state.field18=0;}
 if((a.type10(attribute)&31)==1)return a.data4(attribute);
 if(static_cast<unsigned char>(a.canConvert10362e50(attribute))){a.convert103630a0(attribute,&scratch);return &scratch;}
 return &state.handle;
}
