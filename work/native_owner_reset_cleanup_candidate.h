#pragma once
#include "native_query_cleanup_candidate.h"
#include "studio_owner_resource_cleanup_candidate.h"
#include "studio_owner_reset_cleanup_candidate.h"
// All fields borrow live owned storage. Caller controls synchronization/lifetime.
template<class Resource=IUnknown,class Query=IDirect3DQuery9>
struct NativeOwnerResetCleanupViewCandidateT{
 unsigned char& flags;uint32_t& cleanupDepth;DWORD& owningThreadId;
 Resource*& resource;std::array<Query*,2>& queries;std::array<unsigned char,2>& active;
 unsigned char& resourceQueueActive;std::array<Resource*,20>& queuedResources;std::array<Resource*,5>& fixedResources;
};
template<class Lifecycle>struct NativeOwnerResetCleanupServicesCandidateT{
 Lifecycle& lifecycle;
 template<class Owner>bool inOwningThread(Owner& owner){return lifecycle.currentThreadId()==owner.owningThreadId;}
 bool inMainThread(){return lifecycle.inMainThread();}
 void deferCleanup(bool value){lifecycle.deferCleanup(value);}
 void nestedWarning(uint32_t value){lifecycle.nestedWarning(value);}
 void nestedDiagnostic(){lifecycle.nestedDiagnostic();}
 bool notificationAvailable(){return lifecycle.notificationAvailable();}
 void beginNotification(uint32_t value){lifecycle.beginNotification(value);}
 void endNotification(){lifecycle.endNotification();}
 template<class Owner>void queryCleanup(Owner& owner){NativeQueryCleanupServicesCandidateT<Lifecycle> services{lifecycle};studioQueryCleanupCandidate(owner,services);}
 template<class Owner>void cleanupFirst(Owner& owner){studioOwnerFirstResourceCleanupCandidate(owner,*this);}
 template<class Owner>void cleanupSecond(Owner& owner){studioOwnerSecondResourceCleanupCandidate(owner,*this);}
 template<class Resource>void release(Resource* value){value->Release();}
 void setCleanupMode(unsigned value){lifecycle.setCleanupMode(value);}
 void finishCommandState(){lifecycle.finishCommandState();}
 void publishCleanupEnabled(bool value){lifecycle.publishCleanupEnabled(value);}
};
