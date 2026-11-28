#pragma once
#include <cstdint>

// Server102676b0 over borrowed entity/model services. Keep the two distinct
// model-info queries: both execute when mask bit2 is absent, including when
// the first returns false. Their implementation remains host-owned.
template<class Services>
bool sourceServerStandardTraceRulesCandidate(uintptr_t entity,uint32_t mask,Services& services){
 if(services.isStaticProp(entity))return true;
 const auto base=services.baseEntity(entity);if(!base)return true;
 const auto solid=services.solidType(base);
 const auto index=services.modelIndex(base);
 const auto model=services.modelForIndex(index);
 const auto type=services.modelType(model);
 if(!(type==1&&(solid==1||solid==6))&&!(mask&0x02000000u))return false;
 if(!(mask&2u)){
  const auto first=services.modelFlag30(model);
  const auto second=services.modelFlag34(model);
  if(uint8_t(first)!=0&&uint8_t(second)==0)return false;
 }
 if(!(mask&0x4000u)&&services.moveType(base)==7)return false;
 return true;
}
