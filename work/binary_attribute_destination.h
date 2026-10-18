#pragma once
#include <cstdint>
//10394710..10394748: existing attributes reused only for exact low5bit type.
// Type mismatch returns no destination; value consumption/diagnostics are separate.
template<class Find,class ReadFlags,class Create>
uint32_t recoveredBinaryAttributeDestinationCandidate(uint32_t owner,const char*name,uint8_t wireType,Find find,ReadFlags readFlags,Create create){
 if(owner==0xffffffffu)return 0xffffffffu;
 const auto existing=find(owner,name);
 if(existing==0xffffffffu)return create(owner,name,wireType);
 return (readFlags(existing)&31u)==wireType?existing:0xffffffffu;
}
