#pragma once
#include <cstdint>

// Complete decision ordering of server102677a0. Services own the standard
// rules, ignored-entity relationship test, entity resolution and collision
// policies. Their production implementations require separate verification.
template<class Services>
bool sourceServerSimpleTraceFilterCandidate(uintptr_t entity,uint32_t mask,
 uintptr_t ignored,int32_t group,bool hasExtra,Services& services){
 if(!services.standardRules(entity,mask))return false;
 if(ignored&&!services.passEntity(entity,ignored))return false;
 if(services.isStaticProp(entity))return false;
 const auto base=services.baseEntity(entity);
 if(!base)return false;
 if(!services.entityAllows(base,group,mask))return false;
 const auto targetGroup=services.collisionGroup(base);
 if(!services.groupsAllow(group,targetGroup))return false;
 if(hasExtra&&!services.extraAllows(entity,mask))return false;
 return true;
}
