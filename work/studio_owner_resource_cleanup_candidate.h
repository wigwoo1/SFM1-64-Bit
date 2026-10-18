#pragma once
// Live pointer collections; release callbacks may change later slots.
template<class Collection,class Services>
void studioReleaseCollectionCandidate(Collection& collection,Services& services){
 for(auto& slot:collection){if(slot){services.release(slot);slot=nullptr;}}
}
template<class Owner,class Services>
void studioOwnerFirstResourceCleanupCandidate(Owner& owner,Services& services){
 owner.resourceQueueActive=0;
 studioReleaseCollectionCandidate(owner.queuedResources,services);
}
template<class Owner,class Services>
void studioOwnerSecondResourceCleanupCandidate(Owner& owner,Services& services){
 studioReleaseCollectionCandidate(owner.fixedResources,services);
}
