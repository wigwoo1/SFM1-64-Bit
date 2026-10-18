#pragma once
#include <cstdint>
template<class Owner,class Adapter>auto graphLogLookupCandidate(Owner* owner,std::int32_t index,Adapter& a){
 if(index<0 || index>=a.count68(owner))return a.nullLog();
 auto* element=a.resolveModelSlot18(a.handle5c(owner,index));
 if(!element)return a.nullLog();
 if(!static_cast<unsigned char>(a.filterElementSlot4(element,a.liveSymbol10eb31d4())))return a.nullLog();
 return element;
}
