#include <cstdio>
#include <stdexcept>
#include "native_manager_callback_registry_candidate.h"
static void require(bool value){if(!value)throw std::runtime_error("owned registry mismatch");}
struct Allocator:NativeManagerCallbackAllocatorCandidate{
 bool fail=false,throws=false;unsigned allocations=0,reallocations=0,releases=0;
 void* allocate(std::size_t bytes){++allocations;if(throws)throw std::bad_alloc();return fail?nullptr:NativeManagerCallbackAllocatorCandidate::allocate(bytes);}
 void* reallocate(void* previous,std::size_t bytes){++reallocations;if(throws)throw std::bad_alloc();return fail?nullptr:NativeManagerCallbackAllocatorCandidate::reallocate(previous,bytes);}
 void release(void* previous){if(previous)++releases;NativeManagerCallbackAllocatorCandidate::release(previous);}
};
using Registry=NativeManagerCallbackRegistryCandidateT<Allocator>;static Registry* active=nullptr;static unsigned calls=0;static bool add=false;
static void callback(){++calls;if(add){add=false;active->append(callback);}}
int main(){try{unsigned cases=0;
 for(bool throwing:{false,true}){
  Allocator allocator;{Registry registry{allocator};active=&registry;allocator.fail=true;allocator.throws=throwing;bool caught=false;try{registry.append(callback);}catch(const std::bad_alloc&){caught=true;}require(caught&&registry.size()==0&&registry.capacity()==0);allocator.fail=allocator.throws=false;
  for(unsigned i=0;i<8;++i)registry.append(callback);require(registry.size()==8&&registry.capacity()==8);
  allocator.fail=true;allocator.throws=throwing;caught=false;try{registry.append(callback);}catch(const std::bad_alloc&){caught=true;}require(caught&&registry.size()==8&&registry.capacity()==8);calls=0;registry.dispatch();require(calls==8);
  allocator.fail=allocator.throws=false;add=true;calls=0;registry.dispatch();require(calls==8&&registry.size()==9&&registry.capacity()==16);calls=0;registry.dispatch();require(calls==9);
  require(!registry.remove(nullptr)&&registry.size()==9);require(registry.remove(callback)&&registry.size()==8);calls=0;registry.dispatch();require(calls==8);unsigned removed=0;while(registry.remove(callback))++removed;require(removed==8&&registry.size()==0&&registry.capacity()==16);calls=0;registry.dispatch();require(calls==0);++cases;
  }require(allocator.releases==1);
 }
 Allocator allocator;{Registry registry{allocator};bool caught=false;try{registry.append(nullptr);}catch(const std::invalid_argument&){caught=true;}require(caught&&registry.size()==0&&allocator.allocations==0);++cases;}
 std::printf("owned_manager_registry_cases=%u failure_rollback_and_dispatch_growth=passed\n",cases);
 }catch(const std::exception& error){std::puts(error.what());return 1;}}
