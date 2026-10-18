#pragma once
#include <vector>
#include <stdexcept>
#include "live_reference_identity_lookup.h"
#include "pending_reference_identity_lookup.h"
#include "pending_reference_find_or_append.h"
#include "pending_reference_fast_remove.h"
class RecoveredLiveIdentityRegistryCandidate{
 std::vector<std::vector<uint32_t>> values;
public:
 explicit RecoveredLiveIdentityRegistryCandidate(uint32_t count=4096):values(count){if(!count||(count&(count-1)))throw std::invalid_argument("power-of-two bucket count required");}
 const auto&buckets()const{return values;}
 //10354A80: swap last handle into removed position, then shrink.
 bool erase(uint32_t composite){return removeRecoveredPendingReferenceCandidate(values,composite);}
 template<class ResolveIdentity>
 uint32_t insert(uint32_t handle,ResolveIdentity resolve){
  return recoveredPendingFindOrAppendCandidate(handle,[&](uint32_t key,uint32_t&b,uint32_t&i){return recoveredPendingIdentityLookupCandidate(key,true,uint32_t(values.size()-1),uint32_t(values.size()),b,i,[&](uint32_t h){auto*identity=resolve(h);return identity?(*identity)[0]:0u;},[&](uint32_t bucket){return uint32_t(values[bucket].size());},[&](uint32_t bucket,uint32_t index){return values[bucket][index];},[](uint32_t a,uint32_t b){return a==b;});},[&](uint32_t b){return uint32_t(values[b].size());},[&](uint32_t b,uint32_t i,uint32_t h){if(i>=0xffffu)throw std::length_error("live bucket full");values[b].push_back(h);return i;});
 }
 template<class ResolveIdentity>
 uint32_t findPosition(const RecoveredPendingIdentityCandidate&identity,ResolveIdentity resolve)const{
  uint32_t b=0,i=0;return recoveredLiveIdentityLookupCandidate(identity,true,uint32_t(values.size()-1),uint32_t(values.size()),b,i,[&](uint32_t bucket){return uint32_t(values[bucket].size());},[&](uint32_t bucket,uint32_t index){return values[bucket][index];},resolve)?(b<<16)|i:0xffffffffu;
 }
 template<class ResolveIdentity>
 uint32_t find(const RecoveredPendingIdentityCandidate&identity,ResolveIdentity resolve)const{
  auto position=findPosition(identity,resolve);return position==0xffffffffu?position:values[position>>16][position&0xffffu];
 }
};
