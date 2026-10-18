#pragma once
#include <cstdint>
// Valid caller-managed counts/capacity; allocator failure semantics remain external.
template<class Manager,class Callback,class Services>void studioManagerAppendCandidate(Manager& manager,Callback callback,Services& services){
 const int32_t index=manager.count;
 if(index+1>manager.capacity)services.grow(manager,index-manager.capacity+1);
 ++manager.count;
 manager.cachedBase=manager.callbacks;
 const int32_t following=manager.count-index-1;
 if(following>0)services.move(manager.callbacks+index+1,manager.callbacks+index,following);
 auto* destination=manager.callbacks+index;
 if(destination)*destination=callback;
}
