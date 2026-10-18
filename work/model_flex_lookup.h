#pragma once
#include "mdl_flex_metadata.h"
#include "global_flex_registry.h"
#include "flex_weight_resize.h"
// Native105C2F70 registers visited records, stopping at the first name match.
inline uint32_t recoveredModelFlexLookupCandidate(std::vector<MdlFlexControllerMetadata>* records,const std::string&name,std::vector<uint32_t>& weights,RecoveredGlobalFlexRegistryCandidate* registry){if(!records)return 0xffffffffu;for(auto&c:*records){if(c.diskGlobalIndex==-1&&registry){auto index=registry->registerName(c.name);std::memcpy(&c.diskGlobalIndex,&index,4);}if(recoveredFlexNameEqual(c.name,name)){uint32_t index;std::memcpy(&index,&c.diskGlobalIndex,4);if(index>=weights.size())recoveredFlexWeightResizeCandidate(weights,index+uint32_t(1));return index;}}return 0xffffffffu;}
