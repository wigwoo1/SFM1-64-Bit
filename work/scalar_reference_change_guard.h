#pragma once
#include <cstdint>
//10366000 checks type and changed value before runtime type/permission callbacks.
template<class Resolve,class Matches,class Permission>
bool recoveredScalarReferenceChangeAllowedCandidate(uint8_t attributeType,uint32_t current,uint32_t incoming,uint32_t requiredClass,Resolve resolve,Matches matches,Permission permission){
 if(attributeType!=1||current==incoming)return false;
 if(requiredClass!=0xffffffffu){auto target=resolve(incoming);if(target!=0xffffffffu&&!matches(target,requiredClass))return false;}
 return permission();
}
