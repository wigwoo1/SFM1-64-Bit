#pragma once
#include "pending_reference_identity.h"
//10354710: hash first identity word, resolve stored handles, skip missing elements.
template<class Count,class Handle,class ResolveIdentity>
bool recoveredLiveIdentityLookupCandidate(const RecoveredPendingIdentityCandidate&key,bool masked,uint32_t mask,uint32_t bucketCount,uint32_t&bucket,uint32_t&index,Count count,Handle handle,ResolveIdentity resolveIdentity){
 bucket=masked?(key[0]&mask):(key[0]%bucketCount);
 const uint32_t size=count(bucket);
 for(uint32_t i=0;i<size;++i){auto*identity=resolveIdentity(handle(bucket,i));if(identity&&recoveredPendingIdentityEqualCandidate(key,*identity)){index=i;return true;}}
 return false;
}
