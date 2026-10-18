#pragma once
#include "native_pair_query_recreation_candidate.h"
#include "studio_owner_post_reset_candidate.h"
template<class Lifecycle>struct NativeOwnerPostResetServicesCandidateT{
 Lifecycle& lifecycle;
 template<class Owner>bool inOwningThread(Owner& owner){return lifecycle.currentThreadId()==owner.owningThreadId;}
 bool inMainThread(){return lifecycle.inMainThread();}
 void deferPostReset(unsigned value){lifecycle.deferPostReset(value);}
 void forcedDepthWarning(uint32_t value,const char* reason){lifecycle.forcedDepthWarning(value,reason);}
 void unbalancedWarning(uint32_t value){lifecycle.unbalancedWarning(value);}
 void unbalancedDiagnostic(){lifecycle.unbalancedDiagnostic();}
 template<class Owner>void restoreOwner(Owner& owner,bool value){lifecycle.restoreOwner(owner,value);}
 bool notificationAvailable(){return lifecycle.notificationAvailable();}
 void beginPostNotification(uint32_t value){lifecycle.beginPostNotification(value);}
 void prepareRecreation(){lifecycle.prepareRecreation();}
 template<class Owner>void recreateFirst(Owner& owner){NativePairQueryServicesCandidateT<Lifecycle> services{lifecycle};studioPairQueryRecreationCandidate(owner,services);}
 template<class Owner>void recreateSecond(Owner& owner){NativeQueuedQueryServicesCandidateT<Lifecycle> services{lifecycle};studioQueuedQueryRecreationCandidate(owner,services);}
 void resumeCommandState(){lifecycle.resumeCommandState();}
 void publishInterfaceFactory(){lifecycle.publishInterfaceFactory();}
 void endNotification(){lifecycle.endNotification();}
};
