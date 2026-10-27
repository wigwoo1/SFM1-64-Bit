#pragma once
#include "material_session_collection_candidate.h"
#include "attribute_mutation_gate.h"

// Recovered DmeMaterial name/cache contract; independent of native object layout.
// This is not a DataModel factory, graph cloner, registry, or GC implementation.
// Resource lifetime belongs to services/caller; copying this state struct does
// not clone an element or retain another reference to its resource.
struct MaterialElementBindingCandidate {
 MaterialSessionCachedResourceCandidate binding{0,std::string()};
 uint16_t nameFlags=0x205; // type5 + externally backed member storage
 bool invalidName=true; // original symbol sentinel differs from interned ""
};
inline bool setMaterialElementNameCandidate(MaterialElementBindingCandidate&value,
 const std::string&name,int phase){
 if(name.size()>4095)return false;
 const auto end=name.find('\0');std::string canonical=name.substr(0,end);
 if(!value.invalidName&&value.binding.name&&*value.binding.name==canonical)return true;
 if(!attributeMutationPermitted(value.nameFlags,phase))return true;
 value.binding.name=std::move(canonical);value.invalidName=false;value.nameFlags|=0x4080;return true;
}
struct MaterialElementBindingServicesCandidate:MaterialSessionResourceServicesCandidate {
 virtual void releaseReference(uintptr_t)=0;
};
// Native105C11F0 does not clear the dirty bit. The DataModel owns that step.
// Native10984960(0) leaves a nonnull cache intact when MaterialSystem is absent.
inline void resolveMaterialElementBindingCandidate(MaterialElementBindingCandidate&value,
 bool materialSystemAvailable,MaterialElementBindingServicesCandidate&service){
 if((value.nameFlags&0x80)&&value.binding.resource&&materialSystemAvailable){
  service.releaseReference(value.binding.resource);value.binding.resource=0;
 }
}
