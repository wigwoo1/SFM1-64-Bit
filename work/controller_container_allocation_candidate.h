#pragma once
#include <array>
#include <cstdint>
struct ControllerContainerLayoutCandidate{
 void** buckets;std::uintptr_t compareTag,hashTag;std::array<std::uint32_t,21> fields;
};
template<class Adapter>ControllerContainerLayoutCandidate* allocateControllerContainerCandidate(std::uintptr_t hash,std::uintptr_t compare,Adapter& a){
 auto* object=a.allocateObject();if(!object)return nullptr;
 object->buckets=a.allocateBuckets(16);
 if(!object->buckets){a.releaseObject(object);return nullptr;}
 for(unsigned i=0;i<16;++i)object->buckets[i]=nullptr;
 object->compareTag=compare?compare:0x10a182f0;
 object->hashTag=hash?hash:0x10442c00;
 object->fields.fill(0);object->fields[0]=8;object->fields[1]=16;object->fields[3]=8;object->fields[4]=512;object->fields[5]=256;
 return object;
}
