#pragma once
#include <cstdint>
struct ControlRecordCandidate {
 uint32_t elementHandle=0xffffffffu;
 uint32_t availabilityBits=0;
 uint32_t selectionBits=0;
};
//1008d350: preserve optional writes even when subsequent resolution fails.
// Missing record leaves outputs untouched. Resolver supplies real model lookup.
template<class Element,class MaskForKind,class ResolveHandle>
Element* lookupControlRecordCandidate(int32_t kind,const ControlRecordCandidate* record,
 uint32_t* available,uint32_t* selected,const MaskForKind& maskForKind,
 const ResolveHandle& resolve){
 if(!record)return nullptr;
 if(available)*available=maskForKind(kind)&record->availabilityBits;
 if(selected)*selected=maskForKind(kind)&record->selectionBits;
 return resolve(record->elementHandle);
}
