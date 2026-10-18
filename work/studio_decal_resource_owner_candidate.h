#pragma once
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include "studio_decal_typed_create_candidate.h"
#include "studio_decal_pointer_frontier_candidate.h"
// Owns logical x64 pool blocks and entry arrays. Callers serialize lifecycle
// operations: the original unlock-before-payload initialization is retained.
// Pool mutex alone does not make concurrent create/release safe.
class StudioDecalResourceOwnerCandidate {
 struct Block {StudioDecalPointerBlockCandidate value;std::unique_ptr<StudioDecalPointerNodeCandidate[]> nodes;};
 struct Array {int32_t savedCount;std::unique_ptr<StudioDecalEntryCandidate[]> entries;};
 StudioDecalPointerPoolCandidate pool;
 std::recursive_mutex poolMutex;
 std::vector<std::unique_ptr<Block>> blocks;
 std::unordered_map<uintptr_t,Array> arrays;
 static bool beyond(const StudioDecalPointerPoolCandidate& p,const StudioDecalPointerNodeCandidate* n){return studioDecalPointerBeyondFrontierCandidate(p,n,[](uintptr_t a,uintptr_t b){return a>b;});}
public:
 StudioDecalResourceOwnerCandidate()=default;
 StudioDecalResourceOwnerCandidate(const StudioDecalResourceOwnerCandidate&)=delete;
 StudioDecalResourceOwnerCandidate& operator=(const StudioDecalResourceOwnerCandidate&)=delete;
 template<class Count>
 StudioDecalPointerNodeCandidate* create(uintptr_t descriptor,Count readCount){
  std::unique_lock<std::recursive_mutex> guard(poolMutex,std::defer_lock);
  return studioDecalTypedResourceCreateLiveCandidate(pool,descriptor,readCount,[&]{guard.lock();},[&]{guard.unlock();},[&](auto& state,int n){studioDecalPointerGrowCandidate(state,n,[&](int count){auto b=std::make_unique<Block>();b->nodes=std::make_unique<StudioDecalPointerNodeCandidate[]>(count);b->value.nodes=b->nodes.get();auto* value=&b->value;blocks.push_back(std::move(b));return value;});},[]{throw std::runtime_error("decal pool allocation failed");},beyond,[&](int count){if(count<0)throw std::runtime_error("negative decal allocation count");Array a{count,std::make_unique<StudioDecalEntryCandidate[]>(count)};auto* entries=a.entries.get();arrays.emplace(reinterpret_cast<uintptr_t>(entries),std::move(a));return entries;});
 }
 template<class Associations,class Chain,class FreeBuffer>
 void release(StudioDecalPointerNodeCandidate* node,Associations associations,Chain chain,FreeBuffer freeBuffer){
  std::unique_lock<std::recursive_mutex> guard(poolMutex,std::defer_lock);
  studioDecalTypedResourceReleaseCandidate(pool,node,associations,chain,[&](uintptr_t identity,int flags){auto& a=arrays.at(identity);studioDecalEntryDeleteCandidate(a.entries.get(),a.savedCount,uint32_t(flags),identity,identity,freeBuffer,[&](uintptr_t address){arrays.erase(address);});},[&]{guard.lock();},[&]{guard.unlock();},beyond);
 }
 size_t activeArrayCount()const{return arrays.size();}
 size_t blockCount()const{return blocks.size();}
 const StudioDecalPointerPoolCandidate& poolState()const{return pool;}
};
