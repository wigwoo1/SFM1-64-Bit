#pragma once
#include <cstdint>
struct GraphAttributeDefaultCandidate{std::uint32_t guard=0,handle=0,field14=0,field18=0;};
template<class Element,class Adapter>Element* graphTypedAttributeCandidate(Element* owner,const char* name,std::uintptr_t symbolTag,GraphAttributeDefaultCandidate& state,Adapter& a){
 if(!(state.guard&1)){state.guard|=1;state.handle=0xffffffff;state.field14=0xffffffff;state.field18=0;}
 auto* attribute=a.findAttribute1035dd50(owner,name);
 auto* handle=attribute?a.handleStorage10002780(attribute):&state.handle;
 auto* element=a.resolveModelSlot18(*handle);
 if(!element)return nullptr;
 if(!static_cast<unsigned char>(a.elementTypeSlot4(element,a.liveTypeSymbol(symbolTag))))return nullptr;
 return element;
}
