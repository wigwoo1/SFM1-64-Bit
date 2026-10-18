#pragma once
#include <cstdlib>
#include <cstring>
#include <new>
#include <limits>
#include "studio_manager_append_candidate.h"
#include "studio_manager_growth_candidate.h"
#include "studio_manager_cleanup_candidate.h"
#include "studio_manager_remove_candidate.h"
struct NativeManagerCallbackAllocatorCandidate{
 void* allocate(std::size_t bytes){return std::malloc(bytes);}
 void* reallocate(void* previous,std::size_t bytes){return std::realloc(previous,bytes);}
 void release(void* previous){std::free(previous);}
};
// Owned storage with checked failure recovery. Callers synchronize all access.
// Allocators must preserve previous storage on failed/throwing reallocation.
template<class Allocator=NativeManagerCallbackAllocatorCandidate,class Callback=void(*)()>class NativeManagerCallbackRegistryCandidateT{
 struct Manager{Callback* callbacks=nullptr;int32_t capacity=0,count=0,growBy=0;Callback* cachedBase=nullptr;} manager;
 Allocator& allocator;
 struct Services{
  Allocator& allocator;
  Callback* allocate(std::size_t bytes){return static_cast<Callback*>(allocator.allocate(bytes));}
  Callback* reallocate(Callback* previous,std::size_t bytes){return static_cast<Callback*>(allocator.reallocate(previous,bytes));}
  void grow(Manager& value,int32_t additional){
   const auto previous=value;
   try{studioManagerGrowthCandidate(value,additional,*this);if(!value.callbacks||value.capacity<previous.count+1)throw std::bad_alloc();}
   catch(...){value=previous;throw;}
  }
  void move(Callback* to,Callback* from,int32_t count){std::memmove(to,from,static_cast<std::size_t>(count)*sizeof(Callback));}
 };
public:
 explicit NativeManagerCallbackRegistryCandidateT(Allocator& value):allocator(value){}
 ~NativeManagerCallbackRegistryCandidateT(){allocator.release(manager.callbacks);}
 NativeManagerCallbackRegistryCandidateT(const NativeManagerCallbackRegistryCandidateT&)=delete;
 NativeManagerCallbackRegistryCandidateT& operator=(const NativeManagerCallbackRegistryCandidateT&)=delete;
 void append(Callback callback){
  if(!callback)throw std::invalid_argument("null manager callback");
  if(manager.count==(std::numeric_limits<int32_t>::max)())throw std::length_error("callback registry full");
  Services services{allocator};studioManagerAppendCandidate(manager,callback,services);
 }
 void dispatch(){studioManagerCleanupCandidate(manager);}
 // Restore callbacks borrow live command/current/previous state for the entire
 // dispatch. Capture count once; reload storage after each callback may grow it.
 template<class... Arguments>void dispatchWith(Arguments... arguments){
  const int32_t count=manager.count;
  for(int32_t index=0;index<count;++index)manager.callbacks[index](arguments...);
 }
 // Derived-owner restore clears the whole registration byte and list after a
 // nonempty unsuppressed pass, including entries appended during that pass.
 // If a controlled x64 callback throws, cleanup remains pending for recovery.
 template<class... Arguments>bool dispatchRestoreWith(bool suppressed,uint8_t& flags,Arguments... arguments){
  if(suppressed||manager.count==0)return false;
  dispatchWith(arguments...);
  flags=0;manager.count=0;
  return true;
 }
 // Resource owners unregister outside an active dispatch before destruction.
 bool remove(Callback callback){Services services{allocator};return studioManagerRemoveCandidate(manager,callback,services);}
 int32_t size()const{return manager.count;}
 int32_t capacity()const{return manager.capacity;}
};
